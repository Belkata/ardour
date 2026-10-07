#!/bin/bash
# Compare screenshots against approved baselines.
# usage: visual_diff.sh <baseline-dir> <new-dir> [max-changed-percent]
# For every PNG in <baseline-dir>, writes <new-dir>/diff-<name>.png highlighting
# changed pixels and fails if more than max-changed-percent (default 2) changed.
# Blinking indicators, meters and clocks change between runs, hence the tolerance.
set -u
BASE=$1; NEW=$2; MAX=${3:-2}
FAIL=0
for b in "$BASE"/*.png; do
	n="$NEW/$(basename "$b")"
	[ -f "$n" ] || { echo "[FAIL] missing $(basename "$b")"; FAIL=1; continue; }
	total=$(identify -format "%[fx:w*h]" "$b")
	changed=$(compare -metric AE -fuzz 6% "$b" "$n" "$NEW/diff-$(basename "$b")" 2>&1 >/dev/null | cut -d' ' -f1)
	pct=$(python3 -c "print(round(100.0*float('$changed')/float('$total'),2))")
	if python3 -c "import sys; sys.exit(0 if $pct <= $MAX else 1)"; then
		echo "[ ok ] $(basename "$b"): ${pct}% changed"
	else
		echo "[FAIL] $(basename "$b"): ${pct}% changed (max ${MAX}%) -> diff-$(basename "$b")"; FAIL=1
	fi
done
exit $FAIL
