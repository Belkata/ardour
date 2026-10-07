#!/usr/bin/env python3
"""Static checks for Ardour color themes (gtk2_ardour/themes/*.colors).

  * every file is well-formed and has <Colors>, <ColorAliases>, <Modifiers>
  * no duplicate names; every alias points at an existing palette color
  * strict themes (default: modern, dark) define every alias/modifier that
    the reference theme (dark) defines
  * every color name the C++ code asks for literally, e.g.
    UIConfiguration::instance().color ("alert:green"), is defined by every
    strict theme

usage: check_themes.py [--root <ardour checkout>] [--strict NAME ...]
exit status is non-zero if any strict check fails.
"""

import argparse
import glob
import os
import re
import sys
import xml.etree.ElementTree as ET


def load(path):
    root = ET.parse(path).getroot()
    out = {"colors": {}, "aliases": {}, "modifiers": {}, "errors": []}
    for tag, key, attr in (("Colors", "colors", "value"),
                           ("ColorAliases", "aliases", "alias"),
                           ("Modifiers", "modifiers", "modifier")):
        sect = root.find(tag)
        if sect is None:
            out["errors"].append("missing <%s>" % tag)
            continue
        for el in sect:
            name = el.get("name")
            if name in out[key]:
                out["errors"].append("duplicate %s '%s'" % ({"colors": "color", "aliases": "alias", "modifiers": "modifier"}[key], name))
            out[key][name] = el.get(attr)
    for name, target in out["aliases"].items():
        if target not in out["colors"]:
            out["errors"].append("alias '%s' -> unknown color '%s'" % (name, target))
    return out


COLOR_CALL = re.compile(r'\bcolor\s*\(\s*(?:X_\()?\s*"([^"%]+)"')


def code_color_names(root):
    names = {}
    for sub in ("gtk2_ardour", "libs/widgets", "libs/canvas", "libs/gtkmm2ext"):
        for path in glob.glob(os.path.join(root, sub, "**", "*.cc"), recursive=True):
            with open(path, encoding="utf-8", errors="replace") as f:
                for lineno, line in enumerate(f, 1):
                    if "color" not in line:
                        continue
                    for m in COLOR_CALL.finditer(line):
                        names.setdefault(m.group(1), "%s:%d" % (os.path.relpath(path, root), lineno))
    return names


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--root", default=os.path.join(os.path.dirname(__file__), "..", ".."))
    ap.add_argument("--strict", nargs="*", default=["modern", "dark"])
    args = ap.parse_args()
    root = os.path.abspath(args.root)

    themes = {}
    for path in sorted(glob.glob(os.path.join(root, "gtk2_ardour", "themes", "*-ardour.colors"))):
        name = os.path.basename(path)[:-len("-ardour.colors")]
        try:
            themes[name] = load(path)
        except ET.ParseError as e:
            themes[name] = {"colors": {}, "aliases": {}, "modifiers": {}, "errors": ["XML: %s" % e]}

    ref = themes.get("dark")
    failures = 0
    warnings = 0

    for name, t in themes.items():
        strict = name in args.strict
        problems = list(t["errors"])
        if ref and name != "dark":
            for key in ("aliases", "modifiers"):
                missing = sorted(set(ref[key]) - set(t[key]))
                problems += ["missing %s '%s'" % ({"aliases": "alias", "modifiers": "modifier"}[key], m) for m in missing]
        if problems:
            label = "FAIL" if strict else "warn"
            print("[%s] %s: %d problem(s)" % (label, name, len(problems)))
            for p in problems[:20]:
                print("       ", p)
            if len(problems) > 20:
                print("        ... and %d more" % (len(problems) - 20))
            if strict:
                failures += 1
            else:
                warnings += 1
        else:
            print("[ ok ] %s" % name)

    # color names requested literally by the code
    wanted = code_color_names(root)
    for name in args.strict:
        t = themes.get(name)
        if not t:
            print("[FAIL] strict theme '%s' not found" % name)
            failures += 1
            continue
        defined = set(t["colors"]) | set(t["aliases"])
        missing = sorted(n for n in wanted if n not in defined)
        if missing:
            print("[FAIL] %s: %d color name(s) used in code but not defined" % (name, len(missing)))
            for n in missing:
                print("        '%s' (first used at %s)" % (n, wanted[n]))
            failures += 1
        else:
            print("[ ok ] %s defines all %d color names used literally in code" % (name, len(wanted)))

    print("\n%d failure(s), %d warning(s)" % (failures, warnings))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
