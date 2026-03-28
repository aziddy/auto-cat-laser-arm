#include <Arduino.h>
#include <ESP32Servo.h>
#include <Bluepad32.h>
#include "controller.h"

// Servo pins (from docs/REV1-PROTOBOARD/Wiring.md)
#define SERVO1_PIN 25 // Tilt (Y axis)
#define SERVO2_PIN 26 // Pan (X axis)

// Servo pulse range for MG995-180
#define SERVO_MIN_US 500
#define SERVO_MAX_US 2400

Servo servo1;
Servo servo2;

int lastPanAngle = 90;
int lastTiltAngle = 130;

// ─── Shared Servo Write (used by controller) ───

void writeServos(int panAngle, int tiltAngle) {
  panAngle  = constrain(panAngle, 0, 180);
  tiltAngle = constrain(tiltAngle, 80, 180);

  if (abs(tiltAngle - lastTiltAngle) >= 1) {
    servo1.write(tiltAngle);  // servo1 (pin 25) = tilt
    lastTiltAngle = tiltAngle;
  }
  if (abs(panAngle - lastPanAngle) >= 1) {
    servo2.write(panAngle);   // servo2 (pin 26) = pan
    lastPanAngle = panAngle;
  }
}

// ─── Setup ───

void setup() {
  Serial.begin(115200);
  Serial.println("Cat Laser Controller starting...");

  // Attach servos and center them
  servo1.setPeriodHertz(50);
  servo1.attach(SERVO1_PIN, SERVO_MIN_US, SERVO_MAX_US);
  servo1.write(130); // tilt center

  servo2.setPeriodHertz(50);
  servo2.attach(SERVO2_PIN, SERVO_MIN_US, SERVO_MAX_US);
  servo2.write(90);  // pan center

  Serial.println("Servos attached and centered");

  // Initialize Bluetooth controller (Bluepad32)
  controllerInit();

  Serial.println("To pair controller: hold Xbox/PS button until it blinks fast");
}

// ─── Loop ───

void loop() {
  controllerUpdate();
}
