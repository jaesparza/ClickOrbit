# Password and Jiggler

## Welcome lock

Every boot shows only `Locked`. Enter exactly five left encoder detents followed
by five right detents to reveal the menu. A wrong-direction detent resets progress
and is discarded; try the full sequence again. There is no input timeout. The
successful final detent does not scroll; subsequent turns scroll normally.
Buttons and mouse-jiggle test commands are blocked while locked. Jiggler starts
off. Reboot or the development `reset` command returns to Locked.

The combination is counted in encoder clicks/detents, not complete revolutions.
This is a UI/action lock; firmware storage is not encrypted by this feature.

The K518 S3-side USB connection exposes a keyboard, mouse, and CDC diagnostic
console. Keep the plug in the S3 orientation, not the CH340 sidecar orientation.

- **Password** types the current compile-time demo string `Hello`, without Enter.
- **Password 2** types the other compile-time demo string `World`, without Enter.
- **Jiggler** toggles periodic mouse motion. It starts off after each boot. When
  enabled, it waits five minutes, then sends relative horizontal movements +1 and
  -1, 20 ms apart. The pair repeats every five minutes. Toggling off stops future
  pairs; toggling on restarts the full interval. An in-flight pair completes its
  return movement. No clicks, wheel events, or keyboard strings are sent by Jiggler.
- The rotary ring sends one mouse wheel tick per detent: clockwise scrolls down,
  counter-clockwise scrolls up. It never types a password or toggles Jiggler.
- Jiggler state is displayed only on its button: red `Jiggler off`, orange
  `Jiggler on`. There is no separate status message below it. Password 2 is the
  bottom button; all three buttons fit inside the safe circle.
- The screen uses orange accents; Wi-Fi continues connecting in the background
  but no Wi-Fi name/signal/status is displayed.

USB relative movements are measured in HID counts. Host mouse scaling can affect
the exact number of screen pixels corresponding to one count.

## Build and test

`scripts/build.sh` builds in ESP-IDF 5.3 Docker. `scripts/test.sh` includes pure
direction/key mapping and simulated-clock tests for five-minute scheduling,
repetition, disabling, and re-enabling.

```sh
python3 -m serial.tools.list_ports -v
scripts/flash.sh /dev/cu.usbmodemYOURPORT app
KNOB_PORT=/dev/cu.usbmodemYOURPORT scripts/verify-screens.sh build/verify-jiggler
```

The screen walk exercises the real button callbacks and mouse-wheel ring events.
Focus a disposable text field: its password tests type Hello and World.
It leaves Jiggler off. `jiggle` is a development-only command that queues one real
mouse pair immediately; `state` reports `moves=N`, counting successful report pairs.
`diag` reports reset reason and the diagnostic task's minimum stack headroom.

Current app USB identity: VID `303a`, PID `4009`, product `K518 Keyboard` (two HID
interfaces plus CDC); previous keyboard-only firmware used PID `4005`.
The serial path may change after flashing. ROM identities are `303a:1001` or
`303a:0009`; the flash helper requests ROM mode over CDC. If the ROM connection
drops mid-write, re-list ports and repeat the app flash on the ROM port. If the
application is unresponsive, hold BOOT while reconnecting USB for manual recovery.

The test console has an 8 KB stack: measured button snapshot rendering exceeded
the previous 4 KB allocation. Actual USB work runs separately on a 4 KB worker
with a bounded queue and one-second transfer timeout. The TinyUSB task adds its
own 4 KB stack. No USB I/O or timing waits happen inside touch callbacks.

No actual password has been supplied or added. Arbitrary password punctuation and
keyboard layouts need explicit mappings; this prototype sends the alphabetic demo
only. Hardcoded credentials would be recoverable from firmware/device storage.
Wi-Fi credentials remain in the ignored local secrets header.
