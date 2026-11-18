# My Knob — Project Brief

## Goal

I want to build a device that acts as a keyboard, so I can send predefined strings to my PC. The strings would be passwords that will be hardcoded in the device at compile time.

## Steps
- Make the device show itself to the pc as a keyboard.
- When the outerring rotates to the right, the device sends the string "Hello"
- When the outerring rotates to the left, the device sends the string "World"

## Known project baseline

These facts describe the setup verified during development; update them as it changes.

- Hardware: Guition JC3636K518C / K518, ESP32-S3 with 8 MB PSRAM and 16 MB flash.
- Display: round 360 × 360 touch panel; rotary knob and haptic feedback.
- Firmware: workshop starter, ESP-IDF 5.3 and LVGL 9.
- Current screen: “Hello world”, idle/active state, knob counter, and Wi-Fi status.
- Wi-Fi: successfully joined the configured 2.4 GHz LAN; RSSI sampled every second,
  with “weak” below −75 dBm. Internal heap is logged after the first IP per boot.
- Build: `scripts/build.sh` (Docker).
- Host checks: `scripts/test.sh`.
- Last verified S3 port on this Mac: `/dev/cu.usbmodem11201` (recheck after reconnecting).
- App flash after the initial full flash:
  `scripts/flash.sh /dev/cu.usbmodem11201 app`.
- Device checks:
  `KNOB_PORT=/dev/cu.usbmodem11201 scripts/verify-screens.sh`.
- Board pins belong in `components/board/include/board_jc3636k518c.h`.
- Keep device bring-up, reserved pins, and the proven display init sequence intact
  unless the task explicitly requires changing them.