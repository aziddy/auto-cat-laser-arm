#include <Arduino.h>
#include <Bluepad32.h>
#include "controller.h"

// ─── External (defined in main.cpp) ───
extern void writeServos(int panAngle, int tiltAngle);
extern int lastPanAngle;
extern int lastTiltAngle;

// ─── State ───
static ControllerPtr myController = nullptr;

// ABS / REL mode (REL is default)
static bool absMode = false;
static bool lastLBState = false;

// REL mode: persistent servo position
static float relPan = 90.0f;
static float relTilt = 130.0f;

// Timing
static unsigned long lastFrameTime = 0;
static const unsigned long FRAME_INTERVAL = 33; // ~30 fps

// Tuning
static const float DEADZONE = 0.15f;
static const float REL_SPEED = 2.0f;  // degrees/frame at full stick (~60°/s)
static const float REL_BOOST = 4.0f;  // max speed multiplier when RT fully held

// ─── Bluepad32 Callbacks ───

static void onConnectedController(ControllerPtr ctl) {
  if (myController == nullptr) {
    myController = ctl;
    Serial.printf("Controller connected: %s\n", ctl->getModelName().c_str());
  }
}

static void onDisconnectedController(ControllerPtr ctl) {
  if (myController == ctl) {
    myController = nullptr;
    Serial.println("Controller disconnected");
  }
}

// ─── ABS Mode: stick position → servo angle ───

static void processABS(ControllerPtr ctl) {
  // axisX/Y range: -512 to 511
  float lx = ctl->axisX() / 512.0f;
  float ly = ctl->axisY() / 512.0f;

  if (fabsf(lx) < DEADZONE) lx = 0.0f;
  if (fabsf(ly) < DEADZONE) ly = 0.0f;

  // Center stick → Pan 90°, Tilt 130°
  // Full left → Pan 180, Full right → Pan 0
  // Full up (ly negative) → Tilt 180, Full down → Tilt 80
  int panAngle  = (int)((-lx + 1.0f) * 90.0f);
  int tiltAngle = (int)((-ly + 1.0f) * 50.0f + 80.0f);

  writeServos(panAngle, tiltAngle);
}

// ─── REL Mode: stick controls movement rate ───

static void processREL(ControllerPtr ctl) {
  float lx = ctl->axisX() / 512.0f;
  float ly = ctl->axisY() / 512.0f;

  if (fabsf(lx) < DEADZONE) lx = 0.0f;
  if (fabsf(ly) < DEADZONE) ly = 0.0f;

  // Stick centered → hold position, do nothing
  if (lx == 0.0f && ly == 0.0f) return;

  // RT (throttle 0-1023) = speed boost
  float rt = constrain(ctl->throttle(), 0, 1023) / 1023.0f;
  float speed = REL_SPEED * (1.0f + rt * (REL_BOOST - 1.0f));

  // Accumulate deltas
  relPan  += -lx * speed;
  relTilt += -ly * speed;

  // Clamp to safe servo ranges
  relPan  = constrain(relPan,  0.0f, 180.0f);
  relTilt = constrain(relTilt, 80.0f, 180.0f);

  writeServos((int)relPan, (int)relTilt);
}

// ─── Mode Toggle (LB / L1 button) ───

static void checkModeToggle(ControllerPtr ctl) {
  bool lbPressed = ctl->l1();

  // Rising edge → toggle
  if (lbPressed && !lastLBState) {
    absMode = !absMode;
    Serial.printf("Mode: %s\n", absMode ? "ABS" : "REL");

    // Entering REL → seed position from current servo angles (no jump)
    if (!absMode) {
      relPan  = lastPanAngle;
      relTilt = lastTiltAngle;
    }
  }
  lastLBState = lbPressed;
}

// ─── Public API ───

void controllerInit() {
  Serial.println("Initializing Bluepad32...");
  BP32.setup(&onConnectedController, &onDisconnectedController);
  BP32.forgetBluetoothKeys();  // Uncomment to clear stale pairings if controller won't reconnect
  BP32.enableVirtualDevice(false);
  Serial.println("Bluepad32 ready — put controller in pairing mode");
}

void controllerUpdate() {
  bool dataUpdated = BP32.update();

  unsigned long now = millis();
  if (now - lastFrameTime < FRAME_INTERVAL) return;
  lastFrameTime = now;

  // Debug: print controller state every ~1s
  static unsigned long lastDebug = 0;
  if (now - lastDebug >= 1000) {
    lastDebug = now;
    if (myController) {
      Serial.printf("[DBG] connected=%d hasData=%d dataUpdated=%d "
                    "axisX=%d axisY=%d buttons=0x%04x l1=%d\n",
                    myController->isConnected(),
                    myController->hasData(),
                    dataUpdated,
                    myController->axisX(),
                    myController->axisY(),
                    myController->buttons(),
                    myController->l1());
    } else {
      Serial.println("[DBG] myController is null");
    }
  }

  if (myController && myController->isConnected()) {
    checkModeToggle(myController);
    if (absMode) {
      processABS(myController);
    } else {
      processREL(myController);
    }
  }

  vTaskDelay(1);
}
