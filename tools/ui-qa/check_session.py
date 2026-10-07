#!/usr/bin/env python3
"""Assertions on a session saved by the UI QA smoke test.

usage: check_session.py <session.ardour> <ardour.log> [--expect-quick-add]
"""

import re
import sys
import xml.etree.ElementTree as ET

results = []


def check(ok, what):
    results.append((ok, what))
    print("[%s] %s" % (" ok " if ok else "FAIL", what))


def main():
    path, log = sys.argv[1], sys.argv[2]
    expect_quick_add = "--expect-quick-add" in sys.argv

    root = ET.parse(path).getroot()
    routes = [r.get("name") for r in root.iter("Route")]
    check(any(n and n.lower() == "test" for n in routes) or len(routes) >= 3,
          "imported track present (routes: %s)" % ", ".join(n for n in routes if n))
    check("QA Bus" in routes, "bus created from Lua")
    if expect_quick_add:
        check(any(n and n.startswith("QA Track") for n in routes), "track added through the Quick Add popover")
        qa = [r for r in root.iter("Route") if (r.get("name") or "").startswith("QA Track")]
        armed = False
        for r in qa:
            for c in r.iter("Controllable"):
                if c.get("name") == "rec-enable" and float(c.get("value", "0")) > 0:
                    armed = True
        check(armed, "Quick Add 'Arm for recording' (default on) armed the new track")

    rv = root.find(".//RulerVisibility")
    if rv is None:
        check(False, "RulerVisibility stored in session")
    else:
        on = {k: v in ("1", "yes", "true") for k, v in rv.attrib.items()}
        check(not on.get("timecode") and not on.get("tempo") and not on.get("meter"),
              "default rulers: timecode/tempo/meter hidden (%s)" % rv.attrib)
        check(on.get("marker") and on.get("rangemarker") and on.get("arrangement"),
              "default rulers: markers, ranges and arrangement shown")
        check(on.get("minsec") or on.get("bbt"), "default rulers: one time ruler shown")

    with open(log, encoding="utf-8", errors="replace") as f:
        text = f.read()
    missing_colors = sorted(set(re.findall(r"Color (.+?) not found", text)))
    check(not missing_colors, "no 'Color ... not found' messages %s" % (missing_colors or ""))
    crit = [l for l in text.splitlines() if "CRITICAL" in l]
    check(not crit, "no GTK/GLib CRITICAL messages%s" % ((" (%d, first: %s)" % (len(crit), crit[0])) if crit else ""))
    check("QA: script done" in text, "QA Lua script ran to completion")
    check("Unable to find UI style file" not in text, "UI style (rc) file found")
    missing_fonts = sorted(set(re.findall(r"Cannot find (\S+) (?:TrueType )?font", text)))
    check(not missing_fonts, "bundled fonts found %s" % (missing_fonts or ""))

    failed = sum(1 for ok, _ in results if not ok)
    print("\n%d check(s), %d failed" % (len(results), failed))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
