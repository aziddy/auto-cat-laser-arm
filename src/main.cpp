#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <ESP32Servo.h>
#include "webpage.h"

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
      int a2 = constrain(atoi(comma + 1), 80, 180);

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
