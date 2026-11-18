# My Knob

My Knob is firmware for the Guition JC3636K518C K518, an ESP32-S3 device with a 360 × 360 round touch display, rotary encoder, USB, and DRV2605L haptic motor. It combines USB keyboard and mouse controls with a Pomodoro timer.

## Features

- **Combination lock.** On boot, the screen shows a lock and a progress count. Turn the encoder five detents left, then five right to open the menu. An incorrect turn resets the sequence.
- **Keyboard and mouse.** Home provides two text buttons, **Ambu** and **Home**, which send compile-time strings (`Hello` and `World`) as keyboard input over USB or BLE. On Bluetooth, pair the ESP32-S3 HID peripheral named **myAssistant**. The mouse jiggler and ring scrolling use that connection too. The board's separate audio sidecar may also appear as **TAIJI_KNOB_AUDIO**; it is not the HID peripheral.
- **Mouse controls.** Turn the encoder to scroll. The **Jiggler** button enables a small horizontal mouse movement every five minutes.
- **Jiggler interval.** Configuration offers 1, 5, 10, and 30 minute intervals, plus a clearly labeled two-second test mode. The selected interval is saved across reboots. Changing it restarts the wait until the next movement.
- **Pomodoro timer.** Set a focus duration from 1 to 60 minutes with the encoder, then start or pause it by touch. It schedules five-minute breaks and a 15-minute break after every fourth focus session. Breaks can be skipped. A focus session counts as complete only after the full timer elapses.
- **Session totals.** Open **Totals** from the timer screen to review completed focus sessions. The count persists across reboots and can be reset with a confirmation.
- **Office mode.** Swipe up from the unlocked home screen to open Configuration. Office mode mutes button clicks and uses quieter motor alerts. The mode persists across reboots. **Test alert** plays the active alert profile.
- **Haptic jingle.** Press and hold the timer digits to play a short motor jingle.
- **Wi-Fi services.** The device can join a configured 2.4 GHz network, stream uploads, and expose a small HTTP endpoint. Credentials belong in the ignored local `components/net/include/wifi_secrets.h`, copied from the example file.

## Controls

| Screen | Input | Result |
|---|---|---|
| Locked | Five encoder detents left, then five right | Unlock the menu |
| Home | Tap **Pomodoro** | Open the timer |
| Home | Tap **Ambu** or **Home** | Send the associated demo string over the active USB or BLE keyboard |
| Home | Tap **Jiggler** | Toggle five-minute mouse movement |
| Home | Swipe up | Open Configuration |
| Configuration | Tap **Office mode** | Toggle silent clicks and quieter alerts |
| Configuration | Tap **Test alert** | Preview the current alert |
| Timer | Turn encoder while ready | Adjust minutes |
| Timer | Tap **Start**, **Pause**, or **Resume** | Control the current phase |
| Timer | Tap **Skip break** | Continue to the next focus session |
| Timer | Swipe left or tap **Totals** | View completed focus sessions |
| Totals | Tap **Reset totals** twice | Clear the completed-session count |
| Timer | Press and hold the time | Play the motor jingle |

The interface is touch-operated. The knob has no press action.

## Build and flash

The firmware uses ESP-IDF 5.3 and LVGL 9. The build script uses Docker when `idf.py` is not installed locally.

```sh
cp components/net/include/wifi_secrets.example.h components/net/include/wifi_secrets.h
# Edit wifi_secrets.h with local network settings.
scripts/build.sh
scripts/flash.sh /dev/cu.usbmodemYOURPORT app
```

Use `python3 -m serial.tools.list_ports -v` to identify the ESP32-S3 USB port. The K518 exposes a separate USB serial sidecar in the other plug orientation. The flash helper uses esptool without its stub, as required by this board.

## Tests and device tools

```sh
scripts/test.sh
KNOB_PORT=/dev/cu.usbmodemYOURPORT scripts/testctl.py ping
KNOB_PORT=/dev/cu.usbmodemYOURPORT scripts/testctl.py snap build/screen.png
KNOB_PORT=/dev/cu.usbmodemYOURPORT scripts/verify-screens.sh build/verify
```

Host tests cover portable logic such as encoder decoding, Pomodoro timing, lock combinations, keyboard text mapping, and network request rules. The USB test console can inject input events and capture the LVGL framebuffer.

## Project layout

- `components/app/` — lock, home menu, Configuration, Pomodoro timer, and session totals.
- `components/keyboard/` — USB HID keyboard and mouse reports, text mapping, and jiggler schedule.
- `components/haptic/` — DRV2605L motor controls, timer alerts, and jingle playback.
- `components/ui/` — display, touch, backlight, LVGL setup, encoder input, and fonts.
- `components/board/` — K518 pin map and hardware capabilities.
- `components/net/` — Wi-Fi, HTTP client, streaming upload, and device HTTP server.
- `components/testctl/` — development USB serial console.
- `scripts/` — build, flash, host tests, screen verification, and font generation.
- `test/host/` — host-side Unity tests.
- `design/` — palette, design notes, font sources, and font licenses.

## Security and privacy

The lock screen controls access to the interface; it does not encrypt firmware or stored data. Text sent by the keyboard is compiled into firmware and can be recovered from the device image. Do not use the demo strings for real credentials. Real Wi-Fi credentials are kept in the ignored local secrets header and must not be committed.
