#!/usr/bin/env bash
# Walk the app's states through the serial test console (components/testctl) and record what the
# device shows: the LVGL framebuffer (snap) and, if a camera script is given, the webcam. One
# directory of evidence per run; exits 1 on a model mismatch. Extend the list when you add a state.
#   scripts/verify-screens.sh [out_dir]
#   KNOB_CAM=/path/to/verify-with-webcam/scripts/capture.sh # optional webcam capture per step
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT="${1:-$ROOT/build/verify}"
TC="$ROOT/scripts/testctl.py"
CAM="${KNOB_CAM:-}"
mkdir -p "$OUT"
fail=0

step() {   # step <name> <command...> ; EXPECT holds the expected reply substring
  local name="$1"; shift
  local reply; reply="$(python3 "$TC" "$@")" || { echo "FAIL $name: $reply"; fail=1; return; }
  if [[ "$reply" != *"$EXPECT"* ]]; then echo "FAIL $name: got '$reply', want '$EXPECT'"; fail=1; else echo "ok   $name: $reply"; fi
  python3 "$TC" snap "$OUT/$name.png" >/dev/null
  if [ -n "$CAM" ] && [ -x "$CAM" ]; then "$CAM" "$OUT/$name.cam.jpg" >/dev/null; fi
}

wheel_step() {
  local name="$1" detents="$2" delta="$3" before after target
  before="$(python3 "$TC" state)"
  if [[ "$before" =~ wheel=(-?[0-9]+) ]]; then
    target=$((${BASH_REMATCH[1]} + delta))
  else echo "FAIL $name: missing wheel counter"; fail=1; return; fi
  step "$name" knob "$detents"
  after="$(python3 "$TC" state)"
  if [[ "$after" =~ wheel=(-?[0-9]+) ]] && [[ "${BASH_REMATCH[1]}" -eq "$target" ]]; then
    echo "ok   $name: submitted wheel total=$target"
  else echo "FAIL $name: wheel total, got '$after', expected $target"; fail=1; fi
}

EXPECT="pong"                       python3 "$TC" ping >/dev/null || { echo "device not answering on the test console"; exit 1; }
EXPECT="locked=1 last=0 jiggler=off" step 00-locked reset
EXPECT="locked=1 last=0 jiggler=off" step 01-block-password button 1
EXPECT="locked=1 last=0 jiggler=off" step 02-block-jiggler button 2
EXPECT="locked=1 last=0 jiggler=off" step 03-wrong-direction knob +5
EXPECT="locked=1 last=0 jiggler=off" step 04-five-left knob -5
EXPECT="locked=1 last=0 jiggler=off" step 05-four-right knob +4
EXPECT="locked=0 last=0 jiggler=off" step 06-unlocked knob +1
EXPECT="locked=0 last=0 jiggler=off" wheel_step 07-scroll-down +1 -1
EXPECT="locked=0 last=0 jiggler=on" step 08-jiggler-on button 2
EXPECT="locked=0 last=0 jiggler=off" step 09-jiggler-off button 2
EXPECT="locked=1 last=0 jiggler=off" step 10-relocked button 4

echo; echo "evidence in $OUT"; ls "$OUT"
[ "$fail" -eq 0 ] && echo "ALL SCREEN CHECKS PASSED" || { echo "SCREEN CHECKS FAILED"; exit 1; }
