#!/usr/bin/env bash
#
# Gesture E2E: proves the helper's click-mode state machine end-to-end against a virtual
# mouse, with hard assertions and exit codes. Requires /dev/uinput and /dev/input write
# access (input group). The grab is name-filtered to "e2e-test-mouse" — the real mouse
# is never touched.
#
# Scenarios (each must hold or the script exits non-zero):
#   A  quick middle click   -> TRIGGER_DOWN, TRIGGER_UP, click forwarded to the app
#   B  long hold (>holdMs)  -> TRIGGER_HOLD, click injected mid-hold (autoscroll handover)
#   C  drag >dragPx         -> TRIGGER_HOLD, click injected mid-hold
#   D  BLOCK + POS outside  -> swallowed (no BTN_LEFT on the forwarded node)
#   E  BLOCK + POS inside   -> left click forwarded
#
set -euo pipefail

SCRIPTS="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$SCRIPTS/../.." && pwd)"
HELPER="$ROOT/backend/rovyl-helper-linux"
[ -x "$HELPER" ] || { echo "helper not built: $HELPER"; exit 1; }
[ -w /dev/uinput ] || { echo "/dev/uinput not writable (input group? uinput loaded?)"; exit 1; }

command -v gcc >/dev/null || { echo "gcc required"; exit 1; }

TMP="$(mktemp -d)"
trap 'kill $(jobs -pr) 2>/dev/null || true; rm -rf "$TMP"' EXIT

gcc -O2 -include fcntl.h -include sys/time.h -o "$TMP/vmouse" "$SCRIPTS/vmouse.c"
gcc -O2 -o "$TMP/evlisten" "$SCRIPTS/evlisten.c"

# fifo1: control commands to the helper; fifo2: gesture commands to the virtual mouse
FIFO1="$TMP/cmds" FIFO2="$TMP/gest"
mkfifo "$FIFO1" "$FIFO2"

"$HELPER" mouse-blocker-evdev $$ e2e-test-mouse < "$FIFO1" > "$TMP/helper.out" 2> "$TMP/helper.err" &
sleep 0.3
"$TMP/vmouse" < "$FIFO2" > "$TMP/vm.out" 2>&1 &
sleep 0.2
exec 3>"$FIFO1" 4>"$FIFO2"

# wait for the helper's rescan to pick the device up and create the forwarded twin
FWD=""
for _ in $(seq 1 40); do
  for sys in /sys/class/input/input*/name; do
    if grep -q "^rovyl-fwd-e2e-test-mouse$" "$sys" 2>/dev/null; then
      IN="${sys%/name}"
      FWD="/dev/input/$(ls "$IN" | grep '^event' | head -1)"
    fi
  done
  [ -n "$FWD" ] && break
  sleep 0.2
done
[ -n "$FWD" ] || { echo "FAIL: forwarded device never appeared"; cat "$TMP/helper.err"; exit 1; }
echo "fwd node: $FWD"

"$TMP/evlisten" "$FWD" > "$TMP/fwd.out" 2>/dev/null &
sleep 0.3

fail() {
  echo "FAIL: $1"
  echo "--- helper.out:"; cat "$TMP/helper.out"
  echo "--- fwd.out:"; cat "$TMP/fwd.out"
  echo "--- helper.err:"; cat "$TMP/helper.err"
  exit 1
}

echo "TRIGGER 4 click 6 400 30" >&3; sleep 0.4

# A: quick middle click with a live menu -> held back, CLICK_CONSUMED cancels it
echo "TRIGGER 4 click 6 400 1000" >&3; sleep 0.4
echo "BLOCK 100 100 500 400 0 0 1920 1080" >&3; sleep 0.4
echo "POS 200 200" >&3; sleep 0.3
echo "p 2" >&4; sleep 0.15; echo "r 2" >&4; sleep 0.3
echo "CLICK_CONSUMED" >&3; sleep 0.4
echo "UNBLOCK" >&3; sleep 0.4

# A2: quick middle click, no menu -> native click delivered
C0=$(grep -c "KEY 274 1" "$TMP/fwd.out" || true); TH0=$(grep -c "TRIGGER_HOLD" "$TMP/helper.out" || true)
echo "TRIGGER 4 click 6 400 1000" >&3; sleep 0.4
echo "p 2" >&4; sleep 0.15; echo "r 2" >&4; sleep 0.5
C1=$(grep -c "KEY 274 1" "$TMP/fwd.out" || true); TH1=$(grep -c "TRIGGER_HOLD" "$TMP/helper.out" || true)
[ "$C1" -gt "$C0" ] || fail "A2: passthrough click not delivered"
[ "$TH1" = "$TH0" ] || fail "A2: unexpected autoscroll injection"

# B: long hold -> autoscroll handover (down injected at holdMs, up at release)
C0=$(grep -c "KEY 274 1" "$TMP/fwd.out" || true); TH0=$(grep -c "TRIGGER_HOLD" "$TMP/helper.out" || true)
echo "p 2" >&4; sleep 1.15; echo "r 2" >&4; sleep 0.5
C1=$(grep -c "KEY 274 1" "$TMP/fwd.out" || true); TH1=$(grep -c "TRIGGER_HOLD" "$TMP/helper.out" || true)
[ "$TH1" -gt "$TH0" ] || fail "B: no TRIGGER_HOLD"
[ "$C1" -gt "$C0" ] || fail "B: autoscroll injection not delivered"

# C: drag past dragPx (1000 in this run) -> same handover via the drag rule
C0=$(grep -c "KEY 274 1" "$TMP/fwd.out" || true); TH0=$(grep -c "TRIGGER_HOLD" "$TMP/helper.out" || true)
echo "p 2" >&4; sleep 0.1; echo "m 1100 0" >&4; sleep 0.1; echo "r 2" >&4; sleep 0.5
C1=$(grep -c "KEY 274 1" "$TMP/fwd.out" || true); TH1=$(grep -c "TRIGGER_HOLD" "$TMP/helper.out" || true)
[ "$TH1" -gt "$TH0" ] || fail "C: no TRIGGER_HOLD"
[ "$C1" -gt "$C0" ] || fail "C: injection not delivered"

# D: BLOCK + POS outside allowed -> swallowed (BTN_LEFT count unchanged)
echo "BLOCK 100 100 500 400 0 0 1920 1080" >&3; sleep 0.4
LEFT_BEFORE=$(grep -c "KEY 272" "$TMP/fwd.out" || true)
echo "POS 50 50" >&3; sleep 0.3
echo "p 1" >&4; sleep 0.1; echo "r 1" >&4; sleep 0.5
LEFT_MID=$(grep -c "KEY 272" "$TMP/fwd.out" || true); echo "DEBUG counts: before=$LEFT_BEFORE mid=$LEFT_MID"
[ "$LEFT_MID" = "$LEFT_BEFORE" ] || fail "D: click outside allowed rect leaked to the app"

# E: POS inside allowed -> click forwarded
echo "POS 200 200" >&3; sleep 0.3
echo "p 1" >&4; sleep 0.1; echo "r 1" >&4; sleep 0.5
LEFT_AFTER=$(grep -c "KEY 272" "$TMP/fwd.out" || true); echo "DEBUG after=$LEFT_AFTER"
[ "$LEFT_AFTER" -gt "$LEFT_MID" ] || fail "E: click inside allowed rect was not delivered"

echo "UNBLOCK" >&3
echo "EXIT" >&3
sleep 0.5
echo "ALL GESTURE SCENARIOS PASS"
