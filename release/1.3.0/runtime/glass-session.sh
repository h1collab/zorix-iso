#!/bin/sh
# SPDX-License-Identifier: MIT
set -u
mkdir -p "$HOME/.config/zorix"
export XCURSOR_PATH=/usr/share/icons
export XCURSOR_THEME=ZorixGlass
export XCURSOR_SIZE=32
/usr/bin/zorix-rootpaint >/dev/null 2>&1 || true
openbox --config-file /usr/share/zorix/glass-openbox.xml >"$HOME/.config/zorix/openbox.log" 2>&1 &
obpid=$!
sleep .10
compid=
# The portable framebuffer path already copies every frame in software. Running
# a second compositor there adds latency without improving the Glass shell.
# Native Xorg gets Picom for stable redraw, shadows and tear reduction.
if [ "${ZORIX_RENDER_MODE:-portable}" != portable ]; then
  picom --config /usr/share/zorix/picom.conf --daemon >"$HOME/.config/zorix/picom.log" 2>&1 || true
  compid=$(pgrep -n picom 2>/dev/null || true)
fi
cleanup(){ [ -n "$compid" ] && kill "$compid" 2>/dev/null || true; kill "$obpid" 2>/dev/null || true; wait "$obpid" 2>/dev/null || true; }
trap cleanup EXIT HUP INT TERM
sleep .22
if ! kill -0 "$obpid" 2>/dev/null; then
  echo 'Openbox failed to start:' >>"$HOME/.config/zorix/glass.log"
  tail -n 80 "$HOME/.config/zorix/openbox.log" >>"$HOME/.config/zorix/glass.log" 2>/dev/null || true
  exit 70
fi
export CHROME_LOG_FILE="$HOME/.config/zorix/chromium.log"
python3 /usr/lib/zorix/glass_server.py >>"$HOME/.config/zorix/glass.log" 2>&1
status=$?
if [ "$status" -ne 0 ]; then
  echo "Zorix Glass broker/browser exited with status $status" >>"$HOME/.config/zorix/glass.log"
fi
exit "$status"
