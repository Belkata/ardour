#!/bin/bash
# Headless GUI smoke test for the Ardour UI redesign.
#
# usage:
#   tools/ui-qa/smoke.sh <ardour-tree> <out-dir>             full smoke test
#   tools/ui-qa/smoke.sh <ardour-tree> <out-dir> compat <session.ardour>
#                                    only open an existing session (cross-version check)
#
# Needs: a built tree (./waf), Xvfb, xdotool, ImageMagick (import), python3.
# Creates a session from tools/ui-qa/qa_session.lua (import, bus, every edit
# tool, every page, Quick Add), types a track name into the Quick Add popover,
# saves, and checks the result with check_session.py.

set -u
TREE=$(cd "$1" && pwd); OUT=$2; MODE=${3:-full}; EXISTING=${4:-}
HERE=$(cd "$(dirname "$0")" && pwd)
rm -rf "$OUT"; mkdir -p "$OUT"
OUT=$(cd "$OUT" && pwd)

# fresh config: no first-run wizard, Dummy audio backend started automatically
CFG=$OUT/config; mkdir -p $CFG/ardour9
touch $CFG/ardour9/.a9
echo '<?xml version="1.0" encoding="UTF-8"?><instant><no-memory-warning/></instant>' > $CFG/ardour9/instant.xml
cat > $CFG/ardour9/config <<'XML'
<?xml version="1.0" encoding="UTF-8"?>
<Ardour>
  <Config><Option name="try-autostart-engine" value="1"/></Config>
  <Extra><AudioMIDISetup><EngineStates>
    <State backend="None (Dummy)" driver="Normal Speed" device="Silence" input-device="" output-device="" sample-rate="48000" buffer-size="256" n-periods="2" input-latency="0" output-latency="0" lm-input="" lm-output="" active="1" use-buffered-io="0" midi-option="2 in, 2 out, Silence" lru="1"><MIDIDevices/></State>
  </EngineStates></AudioMIDISetup></Extra>
</Ardour>
XML
export XDG_CONFIG_HOME=$CFG

# a short test tone to import
python3 - "$OUT/test.wav" <<'EOF'
import math, struct, sys, wave
w = wave.open(sys.argv[1], "wb"); w.setnchannels(1); w.setsampwidth(2); w.setframerate(48000)
w.writeframes(b"".join(struct.pack("<h", int(12000 * math.sin(2 * math.pi * 220 * i / 48000))) for i in range(48000 * 4)))
w.close()
EOF
export ARDOUR_QA_WAV=$OUT/test.wav

DISP=:$((70 + RANDOM % 19))
Xvfb $DISP -screen 0 1680x1050x24 -nolisten tcp >/dev/null 2>&1 &
XPID=$!
trap 'kill $XPID 2>/dev/null' EXIT
sleep 2
export DISPLAY=$DISP

snap () { import -window root "$OUT/$1.png" 2>/dev/null; }
alive () { kill -0 $APID 2>/dev/null; }

# Xvfb has no window manager: find windows by title, focus them explicitly
win () { xdotool search --name "$1" 2>/dev/null | head -1; }
focus () { local w=$(win "$1"); [ -n "$w" ] && xdotool windowfocus --sync $w 2>/dev/null; }
MAIN_TITLE=${MAIN_TITLE:-qa}
wait_for_main () {
	for i in $(seq 1 20); do
		sleep 4
		echo "wait $i: $(xdotool search --onlyvisible --name . getwindowname %@ 2>/dev/null | tr '\n' '|')" >> $OUT/steps.txt
		[ -n "$(win "$MAIN_TITLE")" ] && return 0
		alive || return 1
		# engine dialog: Start (Return) -- Autostart doesn't fire on a fresh config
		focus "Audio/MIDI Setup" && xdotool key Return
	done
	return 1
}

cd $TREE/gtk2_ardour
FAIL=0

if [ "$MODE" = compat ]; then
	MAIN_TITLE=$(basename "$EXISTING" .ardour)
	./ardev --no-splash "$EXISTING" > $OUT/ardour.log 2>&1 &
	APID=$!
	if wait_for_main && sleep 8 && alive; then
		snap compat
		echo "[ ok ] opened $(basename "$EXISTING")"
	else
		snap compat-fail
		echo "[FAIL] could not open $(basename "$EXISTING") (see $OUT/ardour.log)"; FAIL=1
	fi
	grep -n "CRITICAL\|not found" $OUT/ardour.log | head
	kill $APID 2>/dev/null; sleep 2; kill -9 $APID 2>/dev/null
	exit $FAIL
fi

SESS=$OUT/session/qa
# --template takes the name of a SessionInit script found in the Lua script path
mkdir -p $CFG/ardour9/scripts; cp $HERE/qa_session.lua $CFG/ardour9/scripts/
./ardev --no-splash --new "$SESS" --template "UI QA smoke session" > $OUT/ardour.log 2>&1 &
APID=$!

if ! wait_for_main; then
	snap startup-fail
	echo "[FAIL] Ardour did not come up (see $OUT/ardour.log)"; exit 1
fi

# the Lua script ends with the Quick Add popover open and its name field focused
for i in $(seq 1 12); do
	grep -q "QA: script done" $OUT/ardour.log && break
	sleep 2
done
sleep 3
snap quick-add
focus "Add Track"
xdotool key ctrl+a; xdotool type --delay 30 "QA Track"; xdotool key Return
sleep 4
snap after-quick-add

alive || { echo "[FAIL] Ardour exited during the test"; FAIL=1; }

# keyboard page switches after the session is up
for k in alt+m alt+c alt+e; do
	focus "$MAIN_TITLE"; xdotool key $k; sleep 2
	alive || { echo "[FAIL] Ardour exited after $k"; FAIL=1; break; }
done
snap final

focus "$MAIN_TITLE"; xdotool key ctrl+s; sleep 4
kill $APID 2>/dev/null; sleep 3; kill -9 $APID 2>/dev/null

python3 $HERE/check_session.py "$SESS/qa.ardour" $OUT/ardour.log --expect-quick-add || FAIL=1
exit $FAIL
