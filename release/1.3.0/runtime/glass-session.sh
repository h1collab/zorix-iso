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
audio_pids=
installerpid=
udiskiepid=
if [ "${ZORIX_RENDER_MODE:-portable}" != portable ]; then
  picom --config /usr/share/zorix/picom.conf --daemon >"$HOME/.config/zorix/picom.log" 2>&1 || true
  compid=$(pgrep -n picom 2>/dev/null || true)
fi
for cmd in pipewire pipewire-pulse wireplumber; do
  if command -v "$cmd" >/dev/null 2>&1; then
    "$cmd" >>"$HOME/.config/zorix/audio-session.log" 2>&1 &
    audio_pids="$audio_pids $!"
  fi
done
if command -v udiskie >/dev/null 2>&1; then
  udiskie --automount >>"$HOME/.config/zorix/udiskie.log" 2>&1 &
  udiskiepid=$!
fi
cleanup(){
  [ -n "$installerpid" ] && kill "$installerpid" 2>/dev/null || true
  [ -n "$udiskiepid" ] && kill "$udiskiepid" 2>/dev/null || true
  for pid in $audio_pids; do kill "$pid" 2>/dev/null || true; done
  [ -n "$compid" ] && kill "$compid" 2>/dev/null || true
  kill "$obpid" 2>/dev/null || true
  wait "$obpid" 2>/dev/null || true
}
trap cleanup EXIT HUP INT TERM
sleep .22
if ! kill -0 "$obpid" 2>/dev/null; then
  echo 'Openbox failed to start:' >>"$HOME/.config/zorix/glass.log"
  tail -n 80 "$HOME/.config/zorix/openbox.log" >>"$HOME/.config/zorix/glass.log" 2>/dev/null || true
  exit 70
fi
if [ -e /etc/zorix-live ] && command -v zorix-installer >/dev/null 2>&1; then
  marker="${XDG_RUNTIME_DIR:-/tmp}/zorix-installer-autostarted"
  if [ ! -e "$marker" ]; then
    : >"$marker"
    (
      ready="${XDG_RUNTIME_DIR:-/tmp}/zorix-glass-ready"
      waited=0
      while [ "$waited" -lt 20 ] && [ ! -s "$ready" ]; do
        sleep 1
        waited=$((waited+1))
      done
      if [ ! -s "$ready" ]; then
        echo "Installer autostart skipped: Glass did not report first-paint readiness within 20s." >&2
        exit 76
      fi
      echo "Glass ready; starting installer autostart." >&2
      printf 'ZORIX_INSTALLER_AUTOSTART:glass-ready\n' >/dev/ttyS0 2>/dev/null || true
      sleep 1
      attempt=1
      while [ "$attempt" -le 4 ]; do
        /usr/bin/zorix-installer --autostart
        rc=$?
        [ "$rc" -ne 75 ] && exit "$rc"
        echo "Installer storage wait attempt $attempt/4" >&2
        attempt=$((attempt+1))
        [ "$attempt" -le 4 ] && sleep 4
      done
      echo "Installer autostart gave up after bounded storage retries." >&2
      exit 75
    ) >>"$HOME/.config/zorix/installer-autostart.log" 2>&1 &
    installerpid=$!
  fi
fi
export CHROME_LOG_FILE="$HOME/.config/zorix/chromium.log"
rm -f "${XDG_RUNTIME_DIR:-/tmp}/zorix-glass-ready"
python3 /usr/lib/zorix/glass_server.py >>"$HOME/.config/zorix/glass.log" 2>&1
status=$?
printf 'ZORIX_GLASS_PROCESS_EXIT:%s\n' "$status" >/dev/ttyS0 2>/dev/null || true
if [ "$status" -ne 0 ]; then
  echo "Zorix Glass broker/browser exited with status $status" >>"$HOME/.config/zorix/glass.log"
  tail -n 80 "$HOME/.config/zorix/glass.log" >/dev/ttyS0 2>/dev/null || true
  tail -n 80 "$HOME/.config/zorix/chromium.log" >/dev/ttyS0 2>/dev/null || true
fi
exit "$status"
