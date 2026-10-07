#!/usr/bin/env python3
"""Check that large areas of a screenshot use colors from the theme.

Every color covering more than --min-area percent of a screenshot must be
within --tolerance (RGB distance) of a theme palette color, or of a palette
color composited over a background surface at one of the alpha values the
theme declares (Ardour draws translucent surfaces that way). A large area in an unknown color usually
means a widget the theme does not reach (stock GTK gray, a hard-coded color).

With --ref (default: dark), an area also fails when the old theme explains it
clearly better than the new one, i.e. it still looks like the old theme.

usage: check_screenshot_colors.py <png>... [--theme modern] [--ref dark] [--root .]
                                 [--min-area 1.5] [--tolerance 6]
Needs ImageMagick (convert). Exit status 1 if any large area is unexplained.
"""

import argparse
import os
import subprocess
import sys
import xml.etree.ElementTree as ET


SURFACES = ("theme:bg", "theme:bg1", "theme:bg2", "neutral:background",
            "neutral:backgroundest", "widget:bg")


def palette(root, theme):
    """Palette colors, plus palette colors composited over the theme's
    background surfaces at the alpha values the theme itself declares in
    <Modifiers> -- that is how Ardour draws translucent things."""
    path = os.path.join(root, "gtk2_ardour", "themes", "%s-ardour.colors" % theme)
    xml = ET.parse(path).getroot()
    cols = {}
    for c in xml.find("Colors"):
        v = c.get("value").lower().replace("0x", "")
        cols[c.get("name")] = tuple(int(v[i:i + 2], 16) for i in (0, 2, 4))
    alphas = {0.5}
    for m in xml.find("Modifiers"):
        mod = m.get("modifier", "")
        if "alpha:" in mod:
            alphas.add(round(float(mod.split("alpha:")[1]), 3))
    known = [(rgb, name) for name, rgb in cols.items()]
    for sname in SURFACES:
        base = cols.get(sname)
        if base is None:
            continue
        for name, rgb in cols.items():
            for a in alphas:
                known.append((tuple(round(rgb[i] * a + base[i] * (1 - a)) for i in range(3)),
                              "%s @%g over %s" % (name, a, sname)))
    known += [((0, 0, 0), "black"), ((255, 255, 255), "white")]
    return known


def histogram(png):
    # quantize a little so antialiasing doesn't fragment areas
    out = subprocess.run(["convert", png, "-depth", "8", "-format", "%c", "histogram:info:-"],
                         capture_output=True, text=True, check=True).stdout
    total = 0
    hist = []
    for line in out.splitlines():
        line = line.strip()
        if ":" not in line or "(" not in line:
            continue
        n = int(line.split(":")[0])
        rgb = line.split("(")[1].split(")")[0].split(",")[:3]
        rgb = tuple(int(float(x)) for x in rgb)
        hist.append((n, rgb))
        total += n
    return hist, total


def dist(a, b):
    return sum((a[i] - b[i]) ** 2 for i in range(3)) ** 0.5


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("pngs", nargs="+")
    ap.add_argument("--theme", default="modern")
    ap.add_argument("--ref", default="dark", help="old theme: areas it explains better than --theme fail")
    ap.add_argument("--root", default=os.path.join(os.path.dirname(__file__), "..", ".."))
    ap.add_argument("--min-area", type=float, default=1.5)
    ap.add_argument("--tolerance", type=float, default=6)
    args = ap.parse_args()

    known = palette(os.path.abspath(args.root), args.theme)
    old = palette(os.path.abspath(args.root), args.ref) if args.ref and args.ref != args.theme else []
    failed = 0
    for png in args.pngs:
        hist, total = histogram(png)
        # merge near-identical colors into areas
        areas = []
        for n, rgb in sorted(hist, reverse=True):
            for a in areas:
                if dist(a[1], rgb) < 6:
                    a[0] += n
                    break
            else:
                areas.append([n, rgb])
        big = [(n * 100.0 / total, rgb) for n, rgb in areas if n * 100.0 / total >= args.min_area]
        bad = []
        print("== %s" % os.path.basename(png))
        for pct, rgb in sorted(big, reverse=True):
            d, name = min((dist(rgb, k), nm) for k, nm in known)
            ok = d <= args.tolerance
            note = ""
            if ok and old:
                do, oname = min((dist(rgb, k), nm) for k, nm in old)
                # the old theme explains this area clearly better: un-themed widget
                if do + 2 < d:
                    ok = False
                    note = "; looks like %s: %s, d=%.0f" % (args.ref, oname, do)
            print("  %5.1f%%  #%02x%02x%02x  %s  (%s, d=%.0f%s)" % (pct, *rgb, "ok  " if ok else "FAIL", name, d, note))
            if not ok:
                bad.append(rgb)
        if bad:
            failed += 1
    print("\n%d screenshot(s) with unexplained large color areas" % failed)
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
