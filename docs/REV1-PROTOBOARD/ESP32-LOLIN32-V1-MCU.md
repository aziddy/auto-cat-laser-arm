# ESP32 WeMos Lolin32 V1 — MCU Reference

Module: **ESP-WROOM-32 (REV1)**
Reference: https://mischianti.org/esp32-wemos-lolin32-high-resolution-pinout-and-specs/

## Specifications

| Spec | Value |
|------|-------|
| Processor | Dual-core Xtensa LX6 @ 240 MHz |
| Flash | 4 MB (DIO) |
| SRAM | 520 KB |
| WiFi | 802.11 b/g/n (2.4 GHz) |
| Bluetooth | 4.2 + BLE |
| Operating Voltage | 3.3V |
| USB | Micro-USB (CH340C USB-to-serial) |
| Battery | LiPo 3.7V via PH-2 2.0mm connector, 500 mA charging |
| Built-in LED | GPIO 5 |
| Buttons | RESET, BOOT (GPIO 0) |
| Upload Speed | 921600 baud |
| Max Sketch Size | 1280 KB |
| Dimensions | 57 × 25.4 mm |
| Weight | 6.1 g |

## GPIO Pinout

All GPIOs broken out on the board. PWM is available on all output-capable pins.

| GPIO | ADC | Touch | PWM | Default Function | Notes |
|------|-----|-------|-----|------------------|-------|
| 0 | ADC2_CH1 | T1 | Yes | — | Strapping pin (LOW = boot mode). Outputs PWM at boot |
| 1 | — | — | Yes | UART0 TX | Used for USB serial output |
| 2 | ADC2_CH2 | T2 | Yes | — | Strapping pin (must be LOW/floating at boot) |
| 3 | — | — | Yes | UART0 RX | Used for USB serial input |
| 4 | ADC2_CH0 | T0 | Yes | — | Safe to use |
| 5 | — | — | Yes | VSPI SS, LED_BUILTIN | Strapping pin (HIGH at boot). Outputs PWM at boot |
| 12 | ADC2_CH5 | T5 | Yes | HSPI MISO | Strapping pin (must be LOW at boot) |
| 13 | ADC2_CH4 | T4 | Yes | HSPI MOSI | Safe to use |
| 14 | ADC2_CH6 | T6 | Yes | HSPI CLK | Outputs PWM at boot |
| 15 | ADC2_CH3 | T3 | Yes | HSPI CS | Strapping pin (must be HIGH at boot). Outputs PWM at boot |
| 16 | — | — | Yes | UART2 RX | Safe to use |
| 17 | — | — | Yes | UART2 TX | Safe to use |
| 18 | — | — | Yes | VSPI CLK | Safe to use |
| 19 | — | — | Yes | VSPI MISO | Safe to use |
| 21 | — | — | Yes | I2C SDA | Safe to use |
| 22 | — | — | Yes | I2C SCL | Safe to use |
| 23 | — | — | Yes | VSPI MOSI | Safe to use |
| 25 | ADC2_CH8 | — | Yes | DAC1 | **Servo 1 signal** |
| 26 | ADC2_CH9 | — | Yes | DAC2 | **Servo 2 signal** |
| 27 | ADC2_CH7 | T7 | Yes | — | Safe to use |
| 32 | ADC1_CH4 | T9 | Yes | — | Safe to use |
| 33 | ADC1_CH5 | T8 | Yes | — | Safe to use |
| 34 | ADC1_CH6 | — | No | — | Input only, no pull-up/down |
| 35 | ADC1_CH7 | — | No | — | Input only, no pull-up/down |
| 36 (VP) | ADC1_CH0 | — | No | — | Input only, no pull-up/down |
| 39 (VN) | ADC1_CH3 | — | No | — | Input only, no pull-up/down |

> **GPIOs 6–11** are connected to the onboard SPI flash — do NOT use.

## Pin Capabilities

### ADC (Analog-to-Digital, 12-bit)

**ADC1 — works with WiFi active:**

| Channel | GPIO |
|---------|------|
| CH0 | 36 (VP) |
| CH3 | 39 (VN) |
| CH4 | 32 |
| CH5 | 33 |
| CH6 | 34 |
| CH7 | 35 |

**ADC2 — disabled when WiFi is active:**

| Channel | GPIO |
|---------|------|
| CH0 | 4 |
| CH1 | 0 |
| CH2 | 2 |
| CH3 | 15 |
| CH4 | 13 |
| CH5 | 12 |
| CH6 | 14 |
| CH7 | 27 |
| CH8 | 25 |
| CH9 | 26 |

> ADC readings are non-linear below 0.1V and above 3.2V.

### DAC (Digital-to-Analog, 8-bit)

| Channel | GPIO |
|---------|------|
| DAC1 | 25 |
| DAC2 | 26 |

### Touch Sensors (Capacitive)

| Touch | GPIO |
|-------|------|
| T0 | 4 |
| T1 | 0 |
| T2 | 2 |
| T3 | 15 |
| T4 | 13 |
| T5 | 12 |
| T6 | 14 |
| T7 | 27 |
| T8 | 33 |
| T9 | 32 |

### Communication Buses

**I2C** (any GPIO can be reassigned):
- Default SDA: GPIO 21
- Default SCL: GPIO 22

**SPI:**

| Bus | MOSI | MISO | CLK | CS |
|-----|------|------|-----|----|
| VSPI | 23 | 19 | 18 | 5 |
| HSPI | 13 | 12 | 14 | 15 |

**UART:**

| Bus | TX | RX | Notes |
|-----|----|----|-------|
| UART0 | 1 | 3 | USB serial (programming/logging) |
| UART1 | 10 | 9 | Connected to flash — reassign pins before use |
| UART2 | 17 | 16 | Free to use |

### RTC GPIOs (Deep Sleep Wake Sources)

GPIO 0, 2, 4, 12–15, 25–27, 32–36, 39

## Pin Restrictions

| Restriction | GPIOs | Details |
|-------------|-------|---------|
| Input only | 34, 35, 36, 39 | No internal pull-up/down, cannot be used as outputs |
| Flash connected | 6–11 | Do NOT use — connected to onboard SPI flash |
| Strapping pins | 0, 2, 5, 12, 15 | Affect boot mode — be careful with external loads |
| PWM at boot | 0, 5, 14, 15 | Output PWM signal briefly during startup |
| HIGH at boot | 1, 3, 5, 14, 15 | These pins are pulled HIGH during boot |
| Max current | All | 40 mA per GPIO pin |

## Project Pin Allocation

| GPIO | Assignment | Alt Functions |
|------|-----------|---------------|
| 25 | Servo 1 signal (MG995-180) | DAC1, ADC2_CH8 |
| 26 | Servo 2 signal (MG995-180) | DAC2, ADC2_CH9 |

See [Wiring.md](Wiring.md) for full wiring details.
