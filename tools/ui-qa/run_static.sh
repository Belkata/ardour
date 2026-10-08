#!/bin/bash
# Fast checks that need no build: theme integrity and contrast.
cd "$(dirname "$0")"
FAIL=0
echo "== themes";   python3 check_themes.py   || FAIL=1
echo; echo "== contrast"; python3 check_contrast.py || FAIL=1
echo; echo "== i18n (strings added on this branch)"; python3 check_i18n.py || FAIL=1
exit $FAIL
