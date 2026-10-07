#!/usr/bin/env python3
"""WCAG contrast checks for an Ardour color theme.

Text pairs must reach 4.5:1 (WCAG AA, normal text). For buttons, Ardour
picks the better of "gtk_foreground" / "gtk_background" as text color
(ArdourButton::get_contrasting_color), so we check the best achievable
ratio against each "<name>: fill" / "fill active" and require 3:1.

A pair only fails if the theme is below the threshold *and* worse than
the reference theme (dark), so pre-existing weaknesses are reported as
warnings rather than blocking.

usage: check_contrast.py [--root <ardour checkout>] [--theme modern] [--ref dark]
"""

import argparse
import os
import sys
import xml.etree.ElementTree as ET

TEXT_PAIRS = [
    ("gtk_foreground", "gtk_background", "window text"),
    ("gtk_foreground", "gtk_bases", "list/entry text"),
    ("gtk_fg_selected", "gtk_bg_selected", "selected list row"),
    ("theme:contrasting clock", "clock: background", "main clock digits"),
    ("neutral:foreground", "neutral:background", "text on track lanes"),
    ("neutral:foreground", "theme:bg1", "text on panels"),
]
BUTTONS = [
    "generic button", "transport button", "mouse mode button", "page switch button",
    "route button", "mixer strip button", "tab button", "zoom button",
    "mute button", "solo button", "record enable button", "primary button",
    "add track row button",
]
TEXT_MIN = 4.5
UI_MIN = 3.0


def load(path):
    root = ET.parse(path).getroot()
    colors = {c.get("name"): c.get("value") for c in root.find("Colors")}
    aliases = {a.get("name"): a.get("alias") for a in root.find("ColorAliases")}
    return colors, aliases


def resolve(theme, name):
    colors, aliases = theme
    v = colors.get(aliases.get(name, name))
    if v is None:
        return None
    v = v.lower().replace("0x", "")
    return tuple(int(v[i:i + 2], 16) / 255.0 for i in (0, 2, 4))


def lum(rgb):
    def ch(c):
        return c / 12.92 if c <= 0.03928 else ((c + 0.055) / 1.055) ** 2.4
    r, g, b = (ch(c) for c in rgb)
    return 0.2126 * r + 0.7152 * g + 0.0722 * b


def ratio(a, b):
    la, lb = lum(a), lum(b)
    hi, lo = max(la, lb), min(la, lb)
    return (hi + 0.05) / (lo + 0.05)


def checks(theme):
    out = {}
    for fg, bg, label in TEXT_PAIRS:
        f, b = resolve(theme, fg), resolve(theme, bg)
        if f and b:
            out[label] = (ratio(f, b), TEXT_MIN)
    fg, bg = resolve(theme, "gtk_foreground"), resolve(theme, "gtk_background")
    for btn in BUTTONS:
        for state in ("fill", "fill active"):
            fill = resolve(theme, "%s: %s" % (btn, state))
            if fill is None:
                continue  # falls back to generic button colors at runtime
            out["%s (%s)" % (btn, state)] = (max(ratio(fg, fill), ratio(bg, fill)), UI_MIN)
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--root", default=os.path.join(os.path.dirname(__file__), "..", ".."))
    ap.add_argument("--theme", default="modern")
    ap.add_argument("--ref", default="dark")
    args = ap.parse_args()
    tdir = os.path.join(os.path.abspath(args.root), "gtk2_ardour", "themes")
    theme = load(os.path.join(tdir, "%s-ardour.colors" % args.theme))
    ref = load(os.path.join(tdir, "%s-ardour.colors" % args.ref))

    got, base = checks(theme), checks(ref)
    fails = warns = 0
    print("%-42s %8s %8s  min" % ("pair", args.theme, args.ref))
    for label, (r, need) in got.items():
        rr = base.get(label, (None,))[0]
        status = "ok"
        if r < need:
            if rr is not None and r + 0.05 < rr:
                status = "FAIL"
                fails += 1
            else:
                status = "warn"
                warns += 1
        print("%-42s %7.2f:1 %7s  %.1f  %s" % (label, r, ("%.2f:1" % rr) if rr else "-", need, status))
    print("\n%d failure(s), %d warning(s)" % (fails, warns))
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
