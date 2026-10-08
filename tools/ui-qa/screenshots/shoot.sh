#!/bin/bash
# Screenshot the editor, mixer and Clips (cue) page of a built Ardour tree,
# using a reproducible demo session. Used for before/after comparisons.
#
# usage: [UI_SCALE=150] [SHOTS="editor mixer cues"] tools/ui-qa/screenshots/shoot.sh <ardour-tree> <label> [out-root]
#   writes <out-root>/<label>/{editor,mixer,cues,cues-playing,cues-queued}.png
#   (out-root default: /tmp/ardour-shots; SHOTS picks the pages, "cues" also
#   captures cues-playing and cues-queued)
#
# The demo session is created once and cached in <out-root>/.demo-cache/<key>
# (key: hash of the demo scripts and UI_SCALE); later runs copy it and only
# start Ardour once. Delete the cache to force a fresh session.
#
# Steps:
#   1. (no cache) creates a session from demo_session.lua (imports 5 synthetic stems,
#      adds a bus), then patches it: shows tracks on the cue page and fills 19 cue
#      slots (Ardour has no Lua API for clip slots)
#   2. opens a copy of the cached session and captures the pages
#
# Needs Xvfb, xdotool, ImageMagick (import), python3. Runs headless: there is
# no window manager, so windows are found by title and focused explicitly.

set -u
TREE=$(cd "$1" && pwd); LABEL=$2; ROOT=${3:-/tmp/ardour-shots}
HERE=$(cd "$(dirname "$0")" && pwd)
SHOTS=${SHOTS:-editor mixer cues}
OUT=$ROOT/$LABEL; rm -rf "$OUT"; mkdir -p "$OUT"
WORK=$ROOT/.work-$LABEL; rm -rf "$WORK"; mkdir -p "$WORK"
CFG=$WORK/config
SNAME=demo
KEY=$(cat "$HERE/demo_session.lua" "$HERE/fill_cues.py" "$HERE/make_demo_audio.py" "$HERE/../preseed_config.sh" | { cat; echo "${UI_SCALE:-}"; } | sha1sum | cut -c1-12)
CACHE=$ROOT/.demo-cache/$KEY
SESS=$WORK/session/$SNAME

"$HERE/../preseed_config.sh" "$CFG" "${UI_SCALE:-}"  # no wizard, Dummy backend, no memlock warning; UI_SCALE=150 for HiDPI
mkdir -p "$CFG/ardour9/scripts"
cp "$HERE/demo_session.lua" "$CFG/ardour9/scripts/"   # --template looks scripts up by name
[ -d "$CACHE" ] || python3 "$HERE/make_demo_audio.py" "$WORK/audio"
export ARDOUR_DEMO_AUDIO=$WORK/audio XDG_CONFIG_HOME=$CFG

DISP=:$((90 + RANDOM % 9))
SW=$((1680 * ${UI_SCALE:-100} / 100)); SH=$((1050 * ${UI_SCALE:-100} / 100))   # screen grows with the UI scale
Xvfb $DISP -screen 0 ${SW}x${SH}x24 -nolisten tcp >/dev/null 2>&1 &
XPID=$!
trap 'kill $XPID 2>/dev/null' EXIT
sleep 2
export DISPLAY=$DISP

# park the pointer in a corner first: no hover highlights or tooltips in the shots
snap () { xdotool mousemove $((SW - 1)) $((SH - 1)) 2>/dev/null; sleep 0.5; import -window root "$OUT/$1.png"; }
main_win () { xdotool search --name "^\\*?$SNAME - " 2>/dev/null | head -1; }
want () { case " $SHOTS " in *" $1 "*) return 0;; esac; return 1; }
focus_main () { local w=$(main_win); [ -n "$w" ] && xdotool windowfocus --sync $w 2>/dev/null; }

# wait for the session window; press Start in the engine dialog when it shows
# (Autostart does not fire on a fresh config)
wait_for_main () {
	local last=
	for i in $(seq 1 200); do
		sleep 0.5
		local w=$(xdotool search --onlyvisible --name . getwindowname %@ 2>/dev/null | tr '\n' '|')
		[ "$w" != "$last" ] && echo "$1 step $i: $w" >> "$OUT/steps.txt" && last=$w
		[ -n "$(main_win)" ] && return 0
		local d=$(xdotool search --name "Audio/MIDI Setup" 2>/dev/null | head -1)
		[ -n "$d" ] && xdotool windowfocus --sync $d 2>/dev/null && xdotool key Return && sleep 1
	done
	return 1
}

cd "$TREE/gtk2_ardour"

# 1. create the demo session once (cached)
if [ ! -d "$CACHE" ]; then
	NEW=$WORK/create/$SNAME
	./ardev --no-splash --new "$NEW" --template "UI demo session" > "$OUT/ardour-create.log" 2>&1 &
	APID=$!
	wait_for_main create || { echo "session window never appeared (see $OUT)"; exit 1; }
	sleep 15   # imports finish after the window shows
	focus_main; xdotool key ctrl+s; sleep 3
	kill $APID 2>/dev/null; sleep 3; kill -9 $APID 2>/dev/null; sleep 1
	python3 "$HERE/fill_cues.py" "$NEW/$SNAME.ardour" >> "$OUT/steps.txt" 2>&1 || exit 1
	# keep the config the first run wrote too: the cue page only shows clip
	# play states when the session opens with it
	mkdir -p "$(dirname "$CACHE")"; rm -rf "$CACHE.tmp" "$CACHE.config"
	cp -a "$CFG" "$CACHE.config" && cp -a "$NEW" "$CACHE.tmp" && mv "$CACHE.tmp" "$CACHE"
else
	rm -rf "$CFG"; cp -a "$CACHE.config" "$CFG"
fi

# 2. open a copy (Ardour writes into the session folder) and shoot
mkdir -p "$(dirname "$SESS")"; cp -a "$CACHE" "$SESS"
./ardev --no-splash "$SESS/$SNAME.ardour" > "$OUT/ardour.log" 2>&1 &
APID=$!
wait_for_main open || { echo "session did not open (see $OUT)"; exit 1; }
sleep 3    # first draw of the waveforms
WID=$(main_win)
[ -n "$WID" ] && xdotool windowsize $WID $SW $SH windowmove $WID 0 0 && sleep 1
want editor && { focus_main; snap editor; }
want mixer && { focus_main; xdotool key alt+m; sleep 1.5; snap mixer; }
if want cues; then
	focus_main; xdotool key alt+c; sleep 1.5; snap cues
	# clip states: launch scene B (F2; launching starts the transport, so no Space),
	# then queue scene D while B plays
	focus_main; xdotool key F2; sleep 4; snap cues-playing
	# (capture at once: the queued clips start at the next bar, < 2 s at 120 bpm;
	#  the pointer is still parked from the previous snap)
	focus_main; xdotool key F4; sleep 0.2; import -window root "$OUT/cues-queued.png"
	focus_main; xdotool key space
fi
kill $APID 2>/dev/null; sleep 1; kill -9 $APID 2>/dev/null
echo "done: $OUT"
