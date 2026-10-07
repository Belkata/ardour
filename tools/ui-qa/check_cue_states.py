#!/usr/bin/env python3
"""Check that clip states are visible on the Clips (cue) page screenshots.

Uses the captures shoot.sh takes of the cue grid:
  cues.png          nothing playing
  cues-playing.png  scene B playing           -> green launch icons
  cues-queued.png   scene B playing, D queued -> amber outlines on every queued clip

Pixels are counted in the cue grid area (left part of the window) in the
theme's play color (alert:green) and queued color (theme:contrasting alt).
Thresholds scale with the screenshot size, so it works for UI_SCALE=150 too.

usage: check_cue_states.py <shots-dir> [--theme modern] [--root .] [--queued-tracks 5]
Needs ImageMagick (convert, identify). Exit status 1 on failure.
"""

import argparse
import os
import subprocess
import sys
import xml.etree.ElementTree as ET


def theme_colors(root, theme):
    path = os.path.join(root, "gtk2_ardour", "themes", "%s-ardour.colors" % theme)
    cols = {}
    for c in ET.parse(path).getroot().find("Colors"):
        v = c.get("value").lower().replace("0x", "")
        cols[c.get("name")] = tuple(int(v[i:i + 2], 16) for i in (0, 2, 4))
    return cols


def count(png, rgb, tol=10):
    """pixels near rgb in the cue grid area: left 40 %, from 10 % to 60 % height"""
    w, h = map(int, subprocess.run(["identify", "-format", "%w %h", png],
                                   capture_output=True, text=True, check=True).stdout.split())
    crop = "%dx%d+0+%d" % (int(w * .4), int(h * .5), int(h * .1))
    out = subprocess.run(["convert", png, "-crop", crop, "+repage", "-depth", "8",
                          "-format", "%c", "histogram:info:-"],
                         capture_output=True, text=True, check=True).stdout
    n = 0
    for line in out.splitlines():
        if "(" not in line:
            continue
        c = [int(float(x)) for x in line.split("(")[1].split(")")[0].split(",")[:3]]
        if sum((c[i] - rgb[i]) ** 2 for i in range(3)) ** .5 <= tol:
            n += int(line.split(":")[0])
    return n, w / 1680.0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("dir")
    ap.add_argument("--theme", default="modern")
    ap.add_argument("--root", default=os.path.join(os.path.dirname(__file__), "..", ".."))
    ap.add_argument("--queued-tracks", type=int, default=5, help="tracks with a clip in the queued scene")
    args = ap.parse_args()

    cols = theme_colors(os.path.abspath(args.root), args.theme)
    green, amber = cols["alert:green"], cols["theme:contrasting alt"]

    shots = {}
    for name in ("cues", "cues-playing", "cues-queued"):
        png = os.path.join(args.dir, name + ".png")
        if not os.path.exists(png):
            print("[FAIL] missing %s" % png)
            return 1
        g, s = count(png, green)
        a, _ = count(png, amber)
        shots[name] = (g, a, s)
        print("  %-13s green %5d  amber %5d" % (name, g, a))

    failed = []
    idle_g, idle_a, s = shots["cues"]
    # a queued outline measures ~500 px at 100 % (2 px around a 105x22 tile); ask for 70 %
    per_tile = 350 * s
    play_g = shots["cues-playing"][0]
    if idle_a > per_tile / 2:
        failed.append("amber outline with nothing queued (%d px)" % idle_a)
    if play_g < idle_g + 150 * s:
        failed.append("no green launch icons while playing (%d px, idle %d)" % (play_g, idle_g))
    if shots["cues-playing"][1] > per_tile / 2:
        failed.append("amber outline in the playing shot, nothing should be queued yet")
    q_a = shots["cues-queued"][1]
    if q_a < per_tile * args.queued_tracks:
        failed.append("queued outlines missing: %d px amber, expected >= %d (%d tracks)"
                      % (q_a, per_tile * args.queued_tracks, args.queued_tracks))
    if shots["cues-queued"][0] < idle_g + 150 * s:
        failed.append("queued shot: the current clips should still be playing")

    for f in failed:
        print("[FAIL]", f)
    print("cue states: %s" % ("FAIL" if failed else "ok"))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
