# RT Speed Boost (REL Mode)

In REL (Relative) mode, holding **RT (Right Trigger)** increases how fast the servos move while the left stick is pushed.

## How It Works

- **RT released** — normal speed: ~2°/frame (~60°/sec at 30fps)
- **RT fully held** — 4x boost: ~8°/frame (~240°/sec at 30fps)
- **RT partially held** — proportional speed between 1x and 4x

The boost is analog — the harder you squeeze RT, the faster the laser moves. This lets you quickly sweep across the room while still having fine control when you ease off the trigger.

## Details

| RT Position | Speed Multiplier | Degrees/sec |
|-------------|-----------------|-------------|
| 0% (released) | 1x | ~60°/s |
| 50% (half) | 2.5x | ~150°/s |
| 100% (full) | 4x | ~240°/s |

Only applies in REL mode. ABS mode maps stick position directly to servo angle, so RT has no effect there.

## Implementation

RT is read via `ctl->throttle()` (0–1023). The speed formula:

```
speed = REL_SPEED * (1.0 + rt * (REL_BOOST - 1.0))
```

Where `REL_SPEED = 2.0` and `REL_BOOST = 4.0`.
