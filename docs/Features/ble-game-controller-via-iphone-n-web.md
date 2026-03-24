# BLE Game Controller via iPhone & Web App

## Feasibility

**Partially feasible — blocked by HTTPS requirement.** An Xbox controller can connect to an iPhone over Bluetooth, and iOS Safari supports the **Web Gamepad API**. However, the Gamepad API requires a **secure context (HTTPS)**, and HTTPS on the ESP32 introduces unacceptable latency for real-time servo control.

### HTTPS Blocker (Tested March 2024)

The Gamepad API (`navigator.getGamepads()`) returns empty arrays on `http://` pages because `window.isSecureContext` is `false`. This was confirmed by testing on iPhone 11 Pro with Xbox One controller.

**What was tried:**
1. **ESP-IDF native HTTPS server** (`esp_https_server.h`) with self-signed cert on port 443
2. **ECC P-256 certificate** (faster than RSA-2048 on ESP32 hardware)
3. **TCP_NODELAY** to disable Nagle's algorithm
4. **Reduced send rate** (10-20fps instead of 30fps)
5. **Tuned httpd config** (task priority, socket count, timeouts)

**Results:** The gamepad was detected and worked over HTTPS, but:
- Page load was slow (~3-5s for TLS handshake, even with ECC)
- WebSocket messages queued up — servo movements were delayed and played back sequentially
- TLS record processing overhead per WebSocket frame was too high for real-time control at any reasonable framerate
- Overall experience was unusable compared to the instant response over plain HTTP

**Conclusion:** HTTPS on ESP32 (LOLIN32, 240MHz, 320KB RAM) is too resource-constrained for real-time WebSocket control. The Gamepad API JS code itself works fine — the blocker is purely the transport layer.

### Possible Future Approaches
- **ESP32-S3 or ESP32-C3** — faster crypto hardware, may handle TLS better
- **BLE HID host on ESP32** — read controller directly via Bluetooth, bypass web Gamepad API entirely
- **Native iOS app** — read controller via GCController framework, send to ESP32 over HTTP
- **Dual-server architecture** — HTTP for fast touch control, HTTPS for slower gamepad mode (user chooses)
- **Wait for browser changes** — if Safari ever relaxes the secure context requirement for local/private IPs

---

The JS integration code below is correct and ready to use — it just needs HTTPS to be viable first.

---

## Compatible Controllers

| Controller | iOS Version Required | Notes |
|---|---|---|
| Xbox One S (Bluetooth) | iOS 13+ | Must be the Bluetooth-capable model |
| Xbox Series X\|S | iOS 14.5+ | |
| PS4 DualShock 4 | iOS 13+ | |
| PS5 DualSense | iOS 14.5+ | |
| Nintendo Joy-Con / Pro Controller | iOS 16+ | |
| MFi controllers (SteelSeries, etc.) | iOS 13+ | |

> Only Bluetooth-capable controllers work. The original Xbox One controller (no Bluetooth) is **not** compatible.

---

## How to Pair

1. Put the controller in pairing mode (e.g. Xbox: hold the **Connect** button until the Xbox button flashes)
2. On iPhone, go to **Settings > Bluetooth**
3. Select the controller from the list
4. Open the CatLaser web app in Safari — the Gamepad API will detect it automatically

---

## Web Gamepad API on iOS Safari

### Support
- Basic support since **Safari 10.1 / iOS 10.3**
- Controller pairing via Bluetooth requires **iOS 13+**
- `navigator.getGamepads()` returns an array of connected gamepads

### Known Quirks on Safari / iOS
- **No partial trigger detection** — analog triggers (L2/R2) read as fully pressed or not pressed (no intermediate values like on Chrome)
- **WKWebView firstResponder** — in embedded webviews (Cordova, etc.) the WKWebView must be firstResponder for the API to work. Not an issue in standalone Safari.
- **Polling required** — the API is polled (call `getGamepads()` each frame), not event-driven in practice

### Standard Gamepad Mapping (axes)
```
axes[0] = Left stick X  (-1 = left, +1 = right)  → Pan
axes[1] = Left stick Y  (-1 = up,   +1 = down)   → Tilt
axes[2] = Right stick X
axes[3] = Right stick Y
```

---

## Integration with Current Web App

### What exists
- Touch joystick on canvas maps to pan/tilt angles
- Angles sent via WebSocket as `"pan,tilt"` string at ~30 FPS
- Pan: 0–180° (or 40–180° with limit), Tilt: 80–180°

### What to add
A `requestAnimationFrame` loop that reads the gamepad and converts stick axes to the same angle format. The existing WebSocket send logic and throttle can be reused.

### Sample JS Integration

```javascript
// Add this alongside the existing touch handling code in webpage.h

let gamepadActive = false;

function pollGamepad() {
  const gamepads = navigator.getGamepads();
  const gp = gamepads[0]; // first connected controller

  if (gp) {
    gamepadActive = true;

    // Left stick axes (with deadzone)
    const deadzone = 0.15;
    let lx = Math.abs(gp.axes[0]) > deadzone ? gp.axes[0] : 0;
    let ly = Math.abs(gp.axes[1]) > deadzone ? gp.axes[1] : 0;

    // Map to servo angles (same as existing touch logic)
    // Pan: stick left (-1) → 180°, stick right (+1) → 40° (or 0° without limit)
    let panAngle, tiltAngle;

    if (document.getElementById('panLimit').checked) {
      panAngle = Math.round((-lx + 1) * 70 + 40);   // 40–180°
    } else {
      panAngle = Math.round((-lx + 1) * 90);         // 0–180°
    }

    // Tilt: stick up (-1) → 180°, stick down (+1) → 80°
    tiltAngle = Math.round((-ly + 1) * 50 + 80);     // 80–180°

    // Update the shared angle variables (reuse existing send logic)
    angle1 = panAngle;
    angle2 = tiltAngle;
  } else {
    gamepadActive = false;
  }

  requestAnimationFrame(pollGamepad);
}

// Start polling when a gamepad connects
window.addEventListener('gamepadconnected', (e) => {
  console.log('Gamepad connected:', e.gamepad.id);
  pollGamepad();
});

window.addEventListener('gamepaddisconnected', (e) => {
  console.log('Gamepad disconnected:', e.gamepad.id);
  gamepadActive = false;
});
```

### Key Integration Notes
- The gamepad loop writes to the same `angle1`/`angle2` variables the touch joystick uses
- The existing throttled WebSocket send (`SEND_INTERVAL = 33ms`) handles transmission — no new send logic needed
- When the gamepad is idle (stick centered), angles stay at center (~110° pan, ~130° tilt)
- Touch input and gamepad input can coexist — whichever was used last "wins"
- A `deadzone` of 0.15 prevents drift from stick noise

---

## Summary

| Aspect | Detail |
|---|---|
| Controller | Xbox Series, PS5 DualSense, etc. via Bluetooth |
| iPhone | iOS 14.5+ recommended |
| Browser | Safari (standalone, not in-app webview) |
| API | `navigator.getGamepads()` — polled each frame |
| ESP32 changes | **None** — same WebSocket protocol |
| Web app changes | ~30 lines of JS for gamepad polling + axis mapping |
| **Blocker** | **Gamepad API requires HTTPS; HTTPS on ESP32 is too slow for real-time control** |
