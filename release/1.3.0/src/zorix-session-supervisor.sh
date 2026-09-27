#!/bin/sh
set -u
state="${XDG_STATE_HOME:-$HOME/.local/state}/zorix"
mkdir -p "$state"
log="$state/session-supervisor.log"
attempt=0
while [ "$attempt" -lt 4 ]; do
  start=$(date +%s)
  echo "$(date -Is) starting Glass session attempt $((attempt+1))" >>"$log"
  /usr/lib/zorix/glass-session.sh >>"$log" 2>&1
  rc=$?
  end=$(date +%s)
  lived=$((end-start))
  echo "$(date -Is) Glass session exited rc=$rc after ${lived}s" >>"$log"
  if [ "$lived" -ge 45 ] && [ "$rc" -eq 0 ]; then exit 0; fi
  attempt=$((attempt+1))
  [ "$attempt" -ge 4 ] && break
  sleep $((1 << (attempt-1)))
done
if command -v zenity >/dev/null 2>&1; then
  zenity --error --title="Zorix desktop recovery" --text="The desktop stopped repeatedly. A terminal will open so you can inspect $log." || true
fi
exec xterm -e sh -lc "tail -n 120 '$log'; echo; echo 'Press Enter to close'; read x"
