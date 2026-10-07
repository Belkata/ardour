#!/bin/bash
# Screenshot the editor, mixer and Clips (cue) page of a built Ardour tree,
# using a reproducible demo session. Used for before/after comparisons.
#
# usage: [UI_SCALE=150] tools/ui-qa/screenshots/shoot.sh <ardour-tree> <label> [out-root]
#   writes <out-root>/<label>/{editor,mixer,cues,cues-playing,cues-queued}.png
#   (out-root default: /tmp/ardour-shots)
#
# Steps:
#   1. creates a session from demo_session.lua (imports 5 synthetic stems, adds a bus)
#   2. patches the saved session: shows tracks on the cue page and fills 19 cue slots
#      (Ardour has no Lua API for clip slots)
#   3. reopens it and captures the three pages
#
# Needs Xvfb, xdotool, ImageMagick (import), python3. Runs headless: there is
# no window manager, so windows are found by title and focused explicitly.

set -u
TREE=$(cd "$1" && pwd); LABEL=$2; ROOT=${3:-/tmp/ardour-shots}
HERE=$(cd "$(dirname "$0")" && pwd)
OUT=$ROOT/$LABEL; rm -rf "$OUT"; mkdir -p "$OUT"
WORK=$ROOT/.work-$LABEL; rm -rf "$WORK"; mkdir -p "$WORK"
CFG=$WORK/config
SESS=$WORK/session/$LABEL

"$HERE/../preseed_config.sh" "$CFG" "${UI_SCALE:-}"  # no wizard, Dummy backend, no memlock warning; UI_SCALE=150 for HiDPI
mkdir -p "$CFG/ardour9/scripts"
cp "$HERE/demo_session.lua" "$CFG/ardour9/scripts/"   # --template looks scripts up by name
python3 "$HERE/make_demo_audio.py" "$WORK/audio"
export ARDOUR_DEMO_AUDIO=$WORK/audio XDG_CONFIG_HOME=$CFG

DISP=:$((90 + RANDOM % 9))
SW=$((1680 * ${UI_SCALE:-100} / 100)); SH=$((1050 * ${UI_SCALE:-100} / 100))   # screen grows with the UI scale
Xvfb $DISP -screen 0 ${SW}x${SH}x24 -nolisten tcp >/dev/null 2>&1 &
XPID=$!
trap 'kill $XPID 2>/dev/null' EXIT
sleep 2
export DISPLAY=$DISP

# park the pointer in a corner first: no hover highlights or tooltips in the shots
snap () { xdotool mousemove $((SW - 1)) $((SH - 1)) 2>/dev/null; sleep 1; import -window root "$OUT/$1.png"; }
main_win () { xdotool search --name "$LABEL" 2>/dev/null | head -1; }
focus_main () { local w=$(main_win); [ -n "$w" ] && xdotool windowfocus --sync $w 2>/dev/null; }

# wait for the session window; press Start in the engine dialog when it shows
# (Autostart does not fire on a fresh config)
wait_for_main () {
	for i in $(seq 1 25); do
		sleep 4
		echo "$1 step $i: $(xdotool search --onlyvisible --name . getwindowname %@ 2>/dev/null | tr '\n' '|')" >> "$OUT/steps.txt"
		[ -n "$(main_win)" ] && return 0
		local d=$(xdotool search --name "Audio/MIDI Setup" 2>/dev/null | head -1)
		[ -n "$d" ] && xdotool windowfocus --sync $d 2>/dev/null && xdotool key Return
	done
	return 1
}

cd "$TREE/gtk2_ardour"

# 1. create
./ardev --no-splash --new "$SESS" --template "UI demo session" > "$OUT/ardour-create.log" 2>&1 &
APID=$!
wait_for_main create || { echo "session window never appeared (see $OUT)"; exit 1; }
sleep 15
focus_main; xdotool key ctrl+s; sleep 3
kill $APID 2>/dev/null; sleep 3; kill -9 $APID 2>/dev/null; sleep 1

# 2. fill cue slots
python3 "$HERE/fill_cues.py" "$SESS/$LABEL.ardour" >> "$OUT/steps.txt" 2>&1

# 3. reopen and shoot
./ardev --no-splash "$SESS/$LABEL.ardour" > "$OUT/ardour.log" 2>&1 &
APID=$!
wait_for_main open || { echo "session did not reopen (see $OUT)"; exit 1; }
sleep 8
WID=$(main_win)
[ -n "$WID" ] && xdotool windowsize $WID $SW $SH windowmove $WID 0 0 && sleep 2
focus_main; snap editor
focus_main; xdotool key alt+m; sleep 4; snap mixer
focus_main; xdotool key alt+c; sleep 4; snap cues
# clip states: launch scene B (F2) and roll, then queue scene C (F3) while it plays
# (the Dummy backend can run well below real time in containers: wait for the bar)
focus_main; xdotool key F2; sleep 0.5; xdotool key space; sleep 10; snap cues-playing
focus_main; xdotool key F3; sleep 0.3; snap cues-queued
focus_main; xdotool key space

kill $APID 2>/dev/null; sleep 2; kill -9 $APID 2>/dev/null
echo "done: $OUT"
