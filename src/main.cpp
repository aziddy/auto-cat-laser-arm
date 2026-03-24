#include <Arduino.h>
#include <WiFi.h>
#include <ESP32Servo.h>
#include <ArduinoWebsockets.h>
#include "config.h"

using namespace websockets;

// Servo pins (from docs/REV1-PROTOBOARD/Wiring.md)
#define SERVO1_PIN 25 // Tilt (Y axis)
#define SERVO2_PIN 26 // Pan (X axis)

// Servo pulse range for MG995-180
#define SERVO_MIN_US 500
#define SERVO_MAX_US 2400

Servo servo1;
Servo servo2;
WebsocketsClient client;

int lastAngle1 = 90;
int lastAngle2 = 90;
unsigned long lastReconnect = 0;
const unsigned long RECONNECT_INTERVAL = 2000;

// ─── WebSocket Message Handler ───

void onMessage(WebsocketsMessage msg) {
  String data = msg.data();
  int comma = data.indexOf(',');
  if (comma > 0) {
    int a1 = constrain(data.substring(0, comma).toInt(), 0, 180);
    int a2 = constrain(data.substring(comma + 1).toInt(), 80, 180);

    // Only write if angle changed (reduces jitter)
    if (abs(a1 - lastAngle1) >= 1) {
      servo2.write(a1);  // servo2 (pin 26) = pan, gets X-axis angle
      lastAngle1 = a1;
    }
    if (abs(a2 - lastAngle2) >= 1) {
      servo1.write(a2);  // servo1 (pin 25) = tilt, gets Y-axis angle
      lastAngle2 = a2;
    }
  }
}

void onEvent(WebsocketsEvent event, String data) {
  if (event == WebsocketsEvent::ConnectionOpened) {
    Serial.println("WebSocket connected to relay");
  } else if (event == WebsocketsEvent::ConnectionClosed) {
    Serial.println("WebSocket disconnected from relay");
  }
}

void connectToRelay() {
  String url = String("ws://") + RELAY_HOST + ":" + RELAY_PORT + "/esp";
  Serial.printf("Connecting to relay: %s\n", url.c_str());
  client.connect(url);
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

  // Connect to WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("Connecting to WiFi: %s", WIFI_SSID);

  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 20000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nWiFi connection failed — restarting");
    ESP.restart();
  }

  Serial.printf("\nWiFi connected — IP: %s\n", WiFi.localIP().toString().c_str());

  // WebSocket client
  client.onMessage(onMessage);
  client.onEvent(onEvent);
  connectToRelay();
}

// ─── Loop ───

void loop() {
  client.poll();

  if (!client.available()) {
    unsigned long now = millis();
    if (now - lastReconnect >= RECONNECT_INTERVAL) {
      lastReconnect = now;
      connectToRelay();
    }
  }
}
