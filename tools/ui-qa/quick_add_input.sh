#!/bin/bash
# Headless check of the Quick Add "Record from" picker and the top bar's solo alert.
#
# usage: tools/ui-qa/quick_add_input.sh <ardour-tree> <out-dir>
#
# Same setup as smoke.sh (Dummy backend: 8 audio inputs). Opens Quick Add, picks
# Stereo and "Input 3 + 4", adds a track named "Picked", solos the first track
# (screenshot solo.png shows the "Solo active" pill), saves, and checks that
# Picked's inputs are connected to exactly system:capture_3 and system:capture_4.
# Clicks use offsets inside the Quick Add window and the first track's S button
# (1680x1050 screen); if the layout changes, check picker-*.png and adjust.
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
win () { xdotool search --onlyvisible --name "$1" 2>/dev/null | head -1; }   # hidden windows share titles
focus () { local w=$(win "$1"); [ -n "$w" ] && xdotool windowfocus --sync $w 2>/dev/null; }
MAIN_TITLE=${MAIN_TITLE:-"qa - Ardour"}   # a modified session gets a "*" prefix
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
# (Lua print() output doesn't reach stdout in the GUI, so wait for the window)
for i in $(seq 1 12); do
	[ -n "$(win "^Add Track$")" ] && break
	sleep 2
done
sleep 3
W=$(win "^Add Track$")
[ -n "$W" ] || { snap no-quick-add; echo "[FAIL] Quick Add window did not open"; exit 1; }
eval $(xdotool getwindowgeometry --shell $W)
focus "^Add Track$"
# Stereo, then open the "Record from" drop-down
xdotool mousemove $((X+175)) $((Y+150)) click 1; sleep 1
xdotool mousemove $((X+130)) $((Y+212)) click 1; sleep 2
snap picker-menu
# items: Automatic, No input, (separator), Input 1 + 2, Input 3 + 4, ...
xdotool key Down Down Down Return; sleep 1
snap picker-chosen
# click into the name field before typing (focus is on the drop-down now)
focus "^Add Track$"; xdotool mousemove $((X+99)) $((Y+111)) click 1; sleep 1
xdotool key ctrl+a; xdotool type --delay 30 "Picked"; xdotool key Return
sleep 4
# solo the first track: the top bar shows "Solo active" only while soloed
focus "$MAIN_TITLE"; xdotool mousemove 209 276 click 1; sleep 3
snap solo
focus "$MAIN_TITLE"; xdotool mousemove 209 276 click 1; sleep 1
focus "$MAIN_TITLE"; xdotool key ctrl+s; sleep 4
alive || { echo "[FAIL] Ardour exited during the test"; FAIL=1; }
kill $APID 2>/dev/null; sleep 2; kill -9 $APID 2>/dev/null

python3 - "$SESS/qa.ardour" <<'PY' || FAIL=1
import sys, xml.etree.ElementTree as ET
root = ET.parse(sys.argv[1]).getroot()
got = {}
for p in root.iter("Port"):
	n = p.get("name", "")
	if n.startswith("Picked/audio_in "):
		got[n] = sorted(c.get("other") for c in p.iter("ExtConnection") if c.get("other"))
want = {"Picked/audio_in 1": ["system:capture_3"], "Picked/audio_in 2": ["system:capture_4"]}
if got == want:
	print("[ ok ] Quick Add 'Record from' Input 3 + 4 -> Picked connected to capture_3/4 only")
else:
	print("[FAIL] Picked inputs: %s, expected %s" % (got, want)); sys.exit(1)
PY
echo "screenshots: $OUT/picker-menu.png $OUT/picker-chosen.png $OUT/solo.png"
exit $FAIL
