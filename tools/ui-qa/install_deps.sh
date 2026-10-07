#!/bin/bash
# Install Ardour's build dependencies plus the tools used by tools/ui-qa
# (Ubuntu 24.04 / noble). Run as root.
#
# Why not just `apt-get install`? In the Claude Code cloud container apt's own
# HTTP downloads stall (~30 s per package through the proxy) while curl is fast,
# and a mirror returns 503s if hit with too many parallel connections. So: ask
# apt for the package URLs, fetch them with curl (4 at a time, with retries),
# then let apt install from its cache without downloading.

set -eu
PKGS="libgtk2.0-dev libgtkmm-2.4-dev libboost-dev libsndfile1-dev libarchive-dev
liblo-dev libtag1-dev libaubio-dev librubberband-dev libfftw3-dev liblilv-dev
libsuil-dev libjack-jackd2-dev libsamplerate0-dev libxml2-dev libcurl4-openssl-dev
libpangomm-1.4-dev libusb-1.0-0-dev libwebsockets-dev vamp-plugin-sdk liblrdf0-dev
libsratom-dev libserd-dev libsord-dev libasound2-dev libpulse-dev libreadline-dev
libudev-dev lv2-dev libglibmm-2.4-dev libcairomm-1.0-dev
xvfb xdotool imagemagick x11-utils gdb"

apt-get update -q
CACHE=/var/cache/apt/archives
apt-get install -y --no-install-recommends --print-uris -qq $PKGS \
	| grep -oE "^'[^']+' [^ ]+" | tr -d "'" > /tmp/ardour-deps-uris.txt

echo "$(wc -l < /tmp/ardour-deps-uris.txt) packages to download"
while read -r url file; do
	[ -f "$CACHE/$file" ] && continue
	echo "$url $file"
done < /tmp/ardour-deps-uris.txt \
	| xargs -P 4 -n 2 sh -c 'curl -sSf --retry 6 --retry-delay 3 --retry-all-errors -o "/var/cache/apt/archives/$1" "$0" || echo "FAILED $1"'

DEBIAN_FRONTEND=noninteractive apt-get install -y -q --no-install-recommends --no-download $PKGS
pkg-config --modversion gtk+-2.0 gtkmm-2.4 lilv-0 rubberband
