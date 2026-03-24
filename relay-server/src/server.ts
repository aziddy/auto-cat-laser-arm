import express from "express";
import { readFileSync } from "node:fs";
import { createServer as createHttpServer } from "node:http";
import { createServer as createHttpsServer } from "node:https";
import { networkInterfaces } from "node:os";
import path from "node:path";
import { WebSocket, WebSocketServer } from "ws";

const HTTPS_PORT = 8443;
const HTTP_PORT = 8080;
const rootDir = path.resolve(__dirname, "..");

// ─── Load TLS certs ───
const tlsOpts = {
  key: readFileSync(path.join(rootDir, "certs", "key.pem")),
  cert: readFileSync(path.join(rootDir, "certs", "cert.pem")),
};

// ─── Express app (serves gamepad UI over HTTPS) ───
const app = express();
app.use(express.static(path.join(rootDir, "public")));

const httpsServer = createHttpsServer(tlsOpts, app);
const browserWss = new WebSocketServer({ noServer: true });

httpsServer.on("upgrade", (req, socket, head) => {
  if (req.url === "/ws") {
    browserWss.handleUpgrade(req, socket, head, (ws) =>
      browserWss.emit("connection", ws, req)
    );
  } else {
    socket.destroy();
  }
});

// ─── Plain HTTP server (ESP32 connects here) ───
const httpServer = createHttpServer();
const espWss = new WebSocketServer({ noServer: true });
let espSocket: WebSocket | null = null;

httpServer.on("upgrade", (req, socket, head) => {
  if (req.url === "/esp") {
    espWss.handleUpgrade(req, socket, head, (ws) =>
      espWss.emit("connection", ws, req)
    );
  } else {
    socket.destroy();
  }
});

espWss.on("connection", (ws) => {
  console.log("ESP32 connected");
  espSocket = ws;
  ws.on("close", () => {
    console.log("ESP32 disconnected");
    if (espSocket === ws) espSocket = null;
  });
});

// ─── Relay: browser → ESP32 ───
browserWss.on("connection", (ws) => {
  console.log("Browser connected");
  ws.on("message", (data) => {
    if (espSocket && espSocket.readyState === WebSocket.OPEN) {
      espSocket.send(data.toString());
    }
  });
  ws.on("close", () => console.log("Browser disconnected"));
});

// ─── Start ───
httpsServer.listen(HTTPS_PORT, () => {
  console.log(`HTTPS server listening on port ${HTTPS_PORT}`);
});
httpServer.listen(HTTP_PORT, () => {
  console.log(`HTTP  server listening on port ${HTTP_PORT} (ESP32)`);
});

// Print local IP for convenience
const localIp = Object.values(networkInterfaces())
  .flat()
  .find((i) => i?.family === "IPv4" && !i.internal)?.address;
if (localIp) {
  console.log(`\nBrowser:  https://${localIp}:${HTTPS_PORT}`);
  console.log(`ESP32:    ws://${localIp}:${HTTP_PORT}/esp`);
}
