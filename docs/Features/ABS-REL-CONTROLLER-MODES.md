# ABS / REL Controller Modes

When a game controller (Xbox, PlayStation, etc.) is connected and enabled, you can toggle between two joystick modes using

## ABS (Absolute) Mode

Stick position maps directly to servo angle. The joystick acts like a coordinate selector.

- **Center stick** = servos move to center position (Pan 90, Tilt 130)
- **Full left** = Pan 180, **Full right** = Pan 0 (or 40-180 with pan limit)
- **Full up** = Tilt 180, **Full down** = Tilt 80
- **Let go of stick** = servos return to center (stick springs back to center, so do the servos)

Good for: quickly snapping to a specific position.

## REL (Relative) Mode — Default

Stick position controls the **rate of movement**, not the position. The joystick acts like a directional pad.

- **Center stick** = servos hold their current position (no movement)
- **Push right** = pan angle decreases (laser moves right) at a speed proportional to how far you push
- **Push left** = pan angle increases (laser moves left)
- **Push up** = tilt angle increases (laser moves up)
- **Push down** = tilt angle decreases (laser moves down)
- **Let go of stick** = servos **stop where they are** and hold position

Speed is ~2 degrees per frame at full stick deflection (~60 degrees/sec at 30fps).

Good for: precise tracking and keeping the laser on a moving target.

## How It Works

| Mode | Function | Behavior |
|------|-------------|----------|
| ABS  | `setAnglesFromStick(lx, ly)` | Computes absolute angles from stick position |
| REL  | `setAnglesFromStickRelative(lx, ly, speed)` | Adds scaled deltas to current angles each frame |

A deadzone of 0.15 is applied to both axes before injection. In REL mode, if both axes are within the deadzone (stick centered), no call is made at all — the servos simply hold position.
