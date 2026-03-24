#pragma once
#include <Arduino.h>

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
  #limit-pan {
    position: fixed; bottom: 36px; left: 0; right: 0;
    text-align: center; color: #555; font: 13px sans-serif;
    z-index: 10;
  }
  #limit-pan input { vertical-align: middle; }
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
<label id="limit-pan"><input type="checkbox" id="panLimit" checked> Limit Pan (40°–180°)</label>
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

    // Map to servo angles
    if (document.getElementById('panLimit').checked) {
      angle1 = Math.round(((-dx / radius) + 1) * 70 + 40);   // full range → 40–180
      angle1 = Math.max(40, Math.min(180, angle1));
    } else {
      angle1 = Math.round(((-dx / radius) + 1) * 90);         // full range → 0–180
      angle1 = Math.max(0, Math.min(180, angle1));
    }
    angle2 = Math.round(((-dy / radius) + 1) * 50 + 80);      // top=180, bottom=80
    angle2 = Math.max(80, Math.min(180, angle2));

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

    if (document.getElementById('panLimit').checked) {
      angle1 = Math.round(((-dx / radius) + 1) * 70 + 40);   // full range → 40–180
      angle1 = Math.max(40, Math.min(180, angle1));
    } else {
      angle1 = Math.round(((-dx / radius) + 1) * 90);         // full range → 0–180
      angle1 = Math.max(0, Math.min(180, angle1));
    }
    angle2 = Math.round(((-dy / radius) + 1) * 50 + 80);      // top=180, bottom=80
    angle2 = Math.max(80, Math.min(180, angle2));

    anglesEl.textContent = 'Pan: ' + angle1 + '\u00B0   Tilt: ' + angle2 + '\u00B0';
    sendAngles();
    requestAnimationFrame(draw);
  }

  // Bridge for native iOS app (GCController → JS)
  // lx: left stick X (-1 left, +1 right), ly: left stick Y (-1 down, +1 up)
  window.setAnglesFromStick = function(lx, ly) {
    var now = Date.now();
    if (now - lastSend < SEND_INTERVAL) return;
    lastSend = now;

    if (document.getElementById('panLimit').checked) {
      angle1 = Math.round((-lx + 1) * 70 + 40);
      angle1 = Math.max(40, Math.min(180, angle1));
    } else {
      angle1 = Math.round((-lx + 1) * 90);
      angle1 = Math.max(0, Math.min(180, angle1));
    }
    angle2 = Math.round((ly + 1) * 50 + 80);
    angle2 = Math.max(80, Math.min(180, angle2));

    thumbX = cx + lx * radius;
    thumbY = cy - ly * radius;
    anglesEl.textContent = 'Pan: ' + angle1 + '\u00B0   Tilt: ' + angle2 + '\u00B0';
    sendAngles();
    requestAnimationFrame(draw);
  };

  // Relative mode: stick controls rate of change, not absolute position
  // lx/ly in [-1, 1], speed in degrees per frame
  window.setAnglesFromStickRelative = function(lx, ly, speed) {
    var now = Date.now();
    if (now - lastSend < SEND_INTERVAL) return;
    lastSend = now;

    angle1 -= Math.round(lx * speed);
    angle2 += Math.round(ly * speed);

    var panMin = document.getElementById('panLimit').checked ? 40 : 0;
    angle1 = Math.max(panMin, Math.min(180, angle1));
    angle2 = Math.max(80, Math.min(180, angle2));

    // Reverse-map angles to thumb position
    var panRange = document.getElementById('panLimit').checked ? 70 : 90;
    var panOffset = document.getElementById('panLimit').checked ? 40 : 0;
    thumbX = cx + ((-(angle1 - panOffset) / panRange + 1)) * radius;
    thumbY = cy - ((angle2 - 80) / 50 - 1) * radius;

    anglesEl.textContent = 'Pan: ' + angle1 + '\u00B0   Tilt: ' + angle2 + '\u00B0';
    sendAngles();
    requestAnimationFrame(draw);
  };
})();
</script>
</body>
</html>
)rawliteral";
