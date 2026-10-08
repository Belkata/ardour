#!/bin/bash
# All GUI checks against a built tree (headless, ~7 minutes):
#   smoke test, Quick Add input picker, screenshots + theme color and clip state checks,
#   HiDPI (150 %) screenshots.
# usage: tools/ui-qa/run_gui.sh [ardour-tree] [out-dir]
HERE=$(cd "$(dirname "$0")" && pwd)
TREE=${1:-$HERE/../..}; OUT=${2:-/tmp/ui-qa-run}
FAIL=0
echo "== smoke";          "$HERE/smoke.sh" "$TREE" "$OUT/smoke" | tail -12 || FAIL=1
[ "${PIPESTATUS[0]}" = 0 ] || FAIL=1
echo; echo "== Quick Add 'Record from' + solo alert"
"$HERE/quick_add_input.sh" "$TREE" "$OUT/quick-add-input" | grep -E "ok|FAIL" ; [ "${PIPESTATUS[0]}" = 0 ] || FAIL=1
echo; echo "== screenshots"; "$HERE/screenshots/shoot.sh" "$TREE" shots "$OUT" || FAIL=1
echo; echo "== theme colors in screenshots"
python3 "$HERE/check_screenshot_colors.py" "$OUT"/shots/{editor,mixer,cues}.png | grep -E "FAIL|unexplained"
[ "${PIPESTATUS[0]}" = 0 ] || FAIL=1
echo; echo "== clip states (playing / queued)"; python3 "$HERE/check_cue_states.py" "$OUT/shots" || FAIL=1
echo; echo "== HiDPI screenshots (150 %)"; UI_SCALE=150 "$HERE/screenshots/shoot.sh" "$TREE" shots-150 "$OUT" || FAIL=1
python3 "$HERE/check_cue_states.py" "$OUT/shots-150" | tail -1; [ "${PIPESTATUS[0]}" = 0 ] || FAIL=1
echo; echo "screenshots: $OUT/shots  $OUT/shots-150"
[ $FAIL = 0 ] && echo "ALL GUI CHECKS PASSED" || echo "SOME GUI CHECKS FAILED"
exit $FAIL
