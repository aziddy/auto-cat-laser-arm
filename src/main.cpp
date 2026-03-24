#include <Arduino.h>
#include <ESP32Servo.h>

#define SERVO2_PIN 26

Servo servo2;

void setup() {
    Serial.begin(115200);
    Serial.println("Servo 2 sweep test");

    servo2.setPeriodHertz(50);
    servo2.attach(SERVO2_PIN, 500, 2400);
}

void loop() {
    for (int angle = 0; angle <= 180; angle++) {
        servo2.write(angle);
        Serial.printf("Angle: %d\n", angle);
        delay(15);
    }

    for (int angle = 180; angle >= 0; angle--) {
        servo2.write(angle);
        Serial.printf("Angle: %d\n", angle);
        delay(15);
    }
}
