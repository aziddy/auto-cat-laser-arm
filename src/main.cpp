#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <ESP32Servo.h>

// WiFi AP settings
const char *AP_SSID = "CatLaser";
const char *AP_PASS = "pew-pew-pew";

// Servo pins (from docs/REV1-PROTOBOARD/Wiring.md)
#define SERVO1_PIN 25 // Tilt (Y axis)
#define SERVO2_PIN 26 // Pan (X axis)

// Servo pulse range for MG995-180
#define SERVO_MIN_US 500
#define SERVO_MAX_US 2400

Servo servo1;
Servo servo2;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
DNSServer dnsServer;

int lastAngle1 = 90;
int lastAngle2 = 90;

// ─── Embedded HTML/CSS/JS Gamepad Page ───

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
<meta name="apple-mobile-web-app-capable" content="yes">
<meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">
<title>Cat Laser</title>
<style>
  * { margin: 0; padding: 0; box-sizing: border-box; }
  html, body {
    width: 100%; height: 100%;
    overflow: hidden;
    background: #1a1a2e;
    touch-action: none;
    -webkit-touch-callout: none;
    -webkit-user-select: none;
    user-select: none;
    overscroll-behavior: none;
    position: fixed;
  }
  #status {
    position: fixed; top: 12px; left: 0; right: 0;
    text-align: center; color: #666; font: 14px sans-serif;
    z-index: 10; pointer-events: none;
  }
  #status.connected { color: #4ecca3; }
  #angles {
    position: fixed; bottom: 12px; left: 0; right: 0;
    text-align: center; color: #444; font: 13px monospace;
    z-index: 10; pointer-events: none;
  }
  #hint {
    position: fixed; top: 40px; left: 0; right: 0;
    text-align: center; color: #333; font: 12px sans-serif;
    z-index: 10; pointer-events: none;
  }
  canvas {
    display: block;
    width: 100%; height: 100%;
    touch-action: none;
  }
</style>
</head>
<body>
<div id="status">Connecting...</div>
<div id="hint">If in popup, open Safari → 192.168.4.1</div>
<div id="angles">Pan: 90° &nbsp; Tilt: 90°</div>
<canvas id="pad"></canvas>
<script>
(function() {
  const canvas = document.getElementById('pad');
  const ctx = canvas.getContext('2d');
  const statusEl = document.getElementById('status');
  const anglesEl = document.getElementById('angles');
  const hintEl = document.getElementById('hint');

  let W, H, cx, cy, radius;
  let thumbX, thumbY;
  let angle1 = 90, angle2 = 90;
  let touching = false;

  function resize() {
    W = canvas.width = window.innerWidth;
    H = canvas.height = window.innerHeight;
    cx = W / 2;
    cy = H / 2;
    radius = Math.min(W, H) * 0.38;
    thumbX = cx;
    thumbY = cy;
    draw();
  }
  window.addEventListener('resize', resize);
  resize();

  function draw() {
    ctx.clearRect(0, 0, W, H);

    // Outer boundary circle
    ctx.beginPath();
    ctx.arc(cx, cy, radius, 0, Math.PI * 2);
    ctx.strokeStyle = '#2a2a4a';
    ctx.lineWidth = 2;
    ctx.stroke();

    // Crosshair
    ctx.strokeStyle = '#2a2a4a';
    ctx.lineWidth = 1;
    ctx.beginPath();
    ctx.moveTo(cx - radius, cy);
    ctx.lineTo(cx + radius, cy);
    ctx.moveTo(cx, cy - radius);
    ctx.lineTo(cx, cy + radius);
    ctx.stroke();

    // Axis labels
    ctx.fillStyle = '#3a3a5a';
    ctx.font = '12px sans-serif';
    ctx.textAlign = 'center';
    ctx.fillText('TILT', cx, cy - radius - 8);
    ctx.save();
    ctx.translate(cx - radius - 8, cy);
    ctx.rotate(-Math.PI / 2);
    ctx.fillText('PAN', 0, 0);
    ctx.restore();

    // Thumb shadow
    ctx.beginPath();
    ctx.arc(thumbX, thumbY, 28, 0, Math.PI * 2);
    ctx.fillStyle = 'rgba(233, 69, 96, 0.15)';
    ctx.fill();

    // Thumb
    ctx.beginPath();
    ctx.arc(thumbX, thumbY, 22, 0, Math.PI * 2);
    ctx.fillStyle = touching ? '#e94560' : '#c73e54';
    ctx.fill();

    // Thumb inner dot
    ctx.beginPath();
    ctx.arc(thumbX, thumbY, 6, 0, Math.PI * 2);
    ctx.fillStyle = '#ff6b81';
    ctx.fill();
  }

  // WebSocket
  let ws_conn;
  let lastSend = 0;
  const SEND_INTERVAL = 33; // ~30fps

  function connect() {
    ws_conn = new WebSocket('ws://' + location.host + '/ws');
    ws_conn.onopen = function() {
      statusEl.textContent = 'Connected';
      statusEl.className = 'connected';
      hintEl.style.display = 'none';
    };
    ws_conn.onclose = function() {
      statusEl.textContent = 'Disconnected — reconnecting...';
      statusEl.className = '';
      setTimeout(connect, 1000);
    };
    ws_conn.onerror = function() { ws_conn.close(); };
  }
  connect();

  function sendAngles() {
    if (ws_conn && ws_conn.readyState === WebSocket.OPEN) {
      ws_conn.send(angle1 + ',' + angle2);
    }
  }

  function handleTouch(e) {
    e.preventDefault();
    var now = Date.now();
    if (now - lastSend < SEND_INTERVAL) return;
    lastSend = now;

    var touch = e.touches[0];
    var dx = touch.clientX - cx;
    var dy = touch.clientY - cy;

    // Clamp to circle
    var dist = Math.sqrt(dx * dx + dy * dy);
    if (dist > radius) {
      dx = dx / dist * radius;
      dy = dy / dist * radius;
    }

    thumbX = cx + dx;
    thumbY = cy + dy;
    touching = true;

    // Map to servo angles (0-180)
    angle1 = Math.round(((dx / radius) + 1) * 90);  // left=0, right=180
    angle2 = Math.round(((-dy / radius) + 1) * 90);  // top=180, bottom=0
    angle1 = Math.max(0, Math.min(180, angle1));
    angle2 = Math.max(0, Math.min(180, angle2));

    anglesEl.textContent = 'Pan: ' + angle1 + '\u00B0   Tilt: ' + angle2 + '\u00B0';
    sendAngles();
    requestAnimationFrame(draw);
  }

  function handleTouchEnd(e) {
    e.preventDefault();
    touching = false;
    requestAnimationFrame(draw);
  }

  canvas.addEventListener('touchstart', handleTouch, { passive: false });
  canvas.addEventListener('touchmove', handleTouch, { passive: false });
  canvas.addEventListener('touchend', handleTouchEnd, { passive: false });
  canvas.addEventListener('touchcancel', handleTouchEnd, { passive: false });

  // Mouse fallback for desktop testing
  var mouseDown = false;
  canvas.addEventListener('mousedown', function(e) { mouseDown = true; handleMouse(e); });
  canvas.addEventListener('mousemove', function(e) { if (mouseDown) handleMouse(e); });
  canvas.addEventListener('mouseup', function() { mouseDown = false; touching = false; draw(); });

  function handleMouse(e) {
    var now = Date.now();
    if (now - lastSend < SEND_INTERVAL) return;
    lastSend = now;

    var dx = e.clientX - cx;
    var dy = e.clientY - cy;
    var dist = Math.sqrt(dx * dx + dy * dy);
    if (dist > radius) { dx = dx / dist * radius; dy = dy / dist * radius; }

    thumbX = cx + dx;
    thumbY = cy + dy;
    touching = true;

    angle1 = Math.round(((dx / radius) + 1) * 90);
    angle2 = Math.round(((-dy / radius) + 1) * 90);
    angle1 = Math.max(0, Math.min(180, angle1));
    angle2 = Math.max(0, Math.min(180, angle2));

    anglesEl.textContent = 'Pan: ' + angle1 + '\u00B0   Tilt: ' + angle2 + '\u00B0';
    sendAngles();
    requestAnimationFrame(draw);
  }
})();
</script>
</body>
</html>
)rawliteral";

// ─── WebSocket Event Handler ───

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
               AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.printf("WebSocket client #%u connected\n", client->id());
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.printf("WebSocket client #%u disconnected\n", client->id());
  } else if (type == WS_EVT_DATA) {
    // Parse "angle1,angle2"
    char msg[len + 1];
    memcpy(msg, data, len);
    msg[len] = '\0';

    char *comma = strchr(msg, ',');
    if (comma) {
      *comma = '\0';
      int a1 = constrain(atoi(msg), 0, 180);
      int a2 = constrain(atoi(comma + 1), 0, 180);

      // Only write if angle changed (reduces jitter)
      if (abs(a2 - lastAngle2) >= 1) {
        servo1.write(a2);  // servo1 (pin 25) = tilt, gets Y-axis angle
        lastAngle2 = a2;
      }
      if (abs(a1 - lastAngle1) >= 1) {
        servo2.write(a1);  // servo2 (pin 26) = pan, gets X-axis angle
        lastAngle1 = a1;
      }
    }
  }
}

// ─── Setup ───

void setup() {
  Serial.begin(115200);
  Serial.println("Cat Laser Controller starting...");

  // Attach servos and center them
  servo1.setPeriodHertz(50);
  servo1.attach(SERVO1_PIN, SERVO_MIN_US, SERVO_MAX_US);
  servo1.write(90);

  servo2.setPeriodHertz(50);
  servo2.attach(SERVO2_PIN, SERVO_MIN_US, SERVO_MAX_US);
  servo2.write(90);

  Serial.println("Servos attached and centered");

  // Start WiFi Access Point
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1),
                     IPAddress(255, 255, 255, 0));
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.printf("AP started: %s\n", AP_SSID);
  Serial.printf("IP: %s\n", WiFi.softAPIP().toString().c_str());

  // DNS server for captive portal — redirect all domains to us
  dnsServer.start(53, "*", WiFi.softAPIP());

  // Serve the gamepad page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html);
  });

  // Captive portal detection endpoints
  server.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html);
  });
  server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html);
  });
  server.on("/connecttest.txt", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html);
  });

  // Redirect everything else to root
  server.onNotFound([](AsyncWebServerRequest *request) {
    request->redirect("/");
  });

  // WebSocket
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  server.begin();
  Serial.println("Web server started — connect to CatLaser WiFi and open 192.168.4.1");
}

// ─── Loop ───

void loop() {
  dnsServer.processNextRequest();
  ws.cleanupClients(2); // Max 2 simultaneous WebSocket clients
}
