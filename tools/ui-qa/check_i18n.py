#!/usr/bin/env python3
"""Check that user-visible strings added on this branch are translatable.

Scans the lines added to gtk2_ardour/ and libs/widgets/ since --base (the
upstream commit the redesign started from) for user-visible text passed as a
bare string literal, i.e. not wrapped in _(), S_(), P_() or X_() (X_ marks
deliberately untranslated strings).

usage: check_i18n.py [--base 608f15a4] [--root .]
"""

import argparse
import os
import re
import subprocess
import sys

# calls whose string arguments are shown to the user
UI_CALL = re.compile(r"\b(?:set_text|set_label|set_markup|set_tip|set_tooltip|set_title|MenuElem|add_button|set_sizing_text)\s*\(")
LITERAL = re.compile(r'"(?:\\.|[^"\\])*"')
WRAPPER_BEFORE = re.compile(r"(?:\b_|\bS_|\bP_|\bX_)\s*\(\s*(?:\"(?:\\.|[^\"\\])*\"\s*,\s*)?$")


def has_words (literal):
    """text left after removing markup tags, %N placeholders and escapes"""
    t = re.sub(r"<[^>]*>", "", literal)
    t = re.sub(r"%[0-9a-z.]*", "", t)
    t = re.sub(r"\\u[0-9a-fA-F]{4}|\\.", "", t)
    return re.search(r"[A-Za-z]{2,}", t) is not None


def added_lines(root, base):
    out = subprocess.run(["git", "-C", root, "diff", "-U0", base, "--", "gtk2_ardour", "libs/widgets"],
                         capture_output=True, text=True, check=True).stdout
    path, lineno = None, 0
    for line in out.splitlines():
        if line.startswith("+++ "):
            path = line[6:] if line.startswith("+++ b/") else None
        elif line.startswith("@@"):
            lineno = int(re.search(r"\+(\d+)", line).group(1))
        elif line.startswith("+") and path and path.endswith((".cc", ".h")):
            yield path, lineno, line[1:]
            lineno += 1


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--base", default="608f15a4")
    ap.add_argument("--root", default=os.path.join(os.path.dirname(__file__), "..", ".."))
    args = ap.parse_args()
    root = os.path.abspath(args.root)

    problems = []
    for path, n, text in added_lines(root, args.base):
        code = text.split("//")[0]
        call = UI_CALL.search(code)
        if not call:
            continue
        for m in LITERAL.finditer(code, call.end()):
            literal = m.group(0)[1:-1]
            if not has_words(literal):
                continue
            if WRAPPER_BEFORE.search(code[:m.start()]):
                continue
            problems.append("%s:%d: \"%s\" -> %s" % (path, n, literal, code.strip()))

    for p in problems:
        print("[FAIL]", p)
    print("%d untranslatable user-visible string(s) added since %s" % (len(problems), args.base))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
