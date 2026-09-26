#!/bin/sh
# SPDX-License-Identifier: MIT
# Zorix OS 1.2.1 volatile Live startup. Internal disks are never auto-mounted.
set -u
fbpid=
xpid=
inputpid=
splash=
export PATH=/usr/sbin:/usr/bin:/sbin:/bin LANG=C.UTF-8
mkdir -p /run/zorix /run/dbus /run/user/1000 /run/zorix-fonts /run/fontconfig /var/log/zorix /var/lib/dbus /tmp/.X11-unix
chmod 1777 /tmp/.X11-unix
chmod 700 /run/user/1000
chown 1000:1000 /run/user/1000
exec >>/var/log/zorix/boot.log 2>&1
printf 'Zorix OS 1.2.1 boot preparation\n'
printf 'Kernel cmdline: '; cat /proc/cmdline

stop_pid() {
  pid="$1"
  [ -z "$pid" ] && return 0
  kill "$pid" 2>/dev/null || return 0
  i=0
  while kill -0 "$pid" 2>/dev/null; do
    if [ "$i" -ge 20 ]; then
      kill -KILL "$pid" 2>/dev/null || true
      break
    fi
    sleep .1
    i=$((i+1))
  done
  wait "$pid" 2>/dev/null || true
}

stop_splash() {
  [ -z "$splash" ] && return 0
  stop_pid "$splash"
  splash=
}

python3 /usr/lib/zorix/ui_typeface.py /run/zorix-fonts/ZorixSans.ttf || true
python3 /usr/lib/zorix/boot_splash.py >/var/log/zorix/splash.log 2>&1 &
splash=$!

for m in \
  evdev usbhid hid_generic xhci_pci ehci_pci uhci_hcd i8042 atkbd psmouse \
  virtio_pci virtio_net virtio_gpu e1000 e1000e r8169 \
  vboxguest vboxvideo vboxsf vmwgfx qxl hyperv_drm hyperv_fb \
  drm drm_kms_helper simpledrm; do
  modprobe "$m" 2>/dev/null || true
done

/usr/lib/systemd/systemd-udevd --daemon || true
udevadm trigger --action=add || true
udevadm settle --timeout=8 || true
dbus-uuidgen --ensure=/etc/machine-id || true
ln -sf /etc/machine-id /var/lib/dbus/machine-id
dbus-daemon --system --fork || true
fc-cache -f /run/zorix-fonts || true

ip link set lo up || true
for p in /sys/class/net/*; do
  name=${p##*/}
  [ "$name" = lo ] && continue
  [ -d "$p/wireless" ] && continue
  ip link set "$name" up || continue
  busybox udhcpc -i "$name" -n -q -t 3 -T 3 -s /usr/lib/zorix/dhcp.sh >"/var/log/zorix/dhcp-$name.log" 2>&1 &
done

recovery() {
  stop_splash
  printf '\033[2J\033[H\033[?25h' >/dev/tty1 2>/dev/null || true
  exec /usr/bin/zorix-recovery-shell /dev/tty1
}

case " $(cat /proc/cmdline) " in
  *' zorix.mode=recovery '*) recovery ;;
esac

missing=0
for c in Xvfb Xorg openbox chromium python3 xauth xdpyinfo dbus-run-session zorix-run-user; do
  if ! command -v "$c" >/dev/null 2>&1; then
    echo "FATAL: required desktop component missing: $c"
    missing=1
  fi
done
[ "$missing" -eq 0 ] || recovery

export DISPLAY=:0 XAUTHORITY=/home/zorix/.Xauthority
cookie=$(od -An -N16 -tx1 /dev/urandom | tr -d ' \n')
touch "$XAUTHORITY"; chmod 600 "$XAUTHORITY"
xauth -f "$XAUTHORITY" add :0 MIT-MAGIC-COOKIE-1 "$cookie" || recovery
chown 1000:1000 "$XAUTHORITY"

mode=auto
case " $(cat /proc/cmdline) " in
  *' zorix.mode=native '*) mode=native ;;
  *' zorix.mode=portable '*) mode=portable ;;
esac

virt=unknown
for f in /sys/class/dmi/id/product_name /sys/class/dmi/id/sys_vendor; do
  [ -r "$f" ] || continue
  v=$(tr -d '\000\r\n' < "$f" 2>/dev/null || true)
  case "$v" in
    *VirtualBox*) virt=virtualbox ; break ;;
    *VMware*) virt=vmware ; break ;;
    *QEMU*|*KVM*) virt=kvm ; break ;;
  esac
done
echo "Virtualization hint: $virt"

wait_x() {
  limit=${1:-40}
  i=0
  while [ "$i" -lt "$limit" ]; do
    xdpyinfo >/dev/null 2>&1 && return 0
    [ -n "$xpid" ] && kill -0 "$xpid" 2>/dev/null || return 1
    sleep .2
    i=$((i+1))
  done
  return 1
}

if [ "$mode" = auto ] || [ "$mode" = native ]; then
  echo 'Trying direct Xorg display server (8 second deadline)'
  stop_splash
  Xorg :0 -auth "$XAUTHORITY" -nolisten tcp -noreset vt1 >/var/log/zorix/xorg.log 2>&1 &
  xpid=$!
  if wait_x 40; then
    mode=native
    echo 'Direct Xorg ready.'
  else
    echo 'Direct Xorg unavailable or timed out; switching to portable display mode.'
    stop_pid "$xpid"
    xpid=
    mode=portable
  fi
fi

if [ "$mode" = portable ]; then
  echo 'Starting portable Xvfb display server'
  size=$(zorix-framebuffer --size 2>/dev/null) || size=1024x768
  case "$size" in *x*) : ;; *) size=1024x768 ;; esac
  Xvfb :0 -screen 0 "${size}x24" -auth "$XAUTHORITY" -nolisten tcp -noreset +extension XTEST >/var/log/zorix/xvfb.log 2>&1 &
  xpid=$!
  if ! wait_x 40; then
    echo 'Portable Xvfb failed.'
    recovery
  fi
  stop_splash
  printf '\033[9;0]' >/dev/tty1 2>/dev/null || true
  zorix-framebuffer >/var/log/zorix/framebuffer.log 2>&1 &
  fbpid=$!
  sleep .15
  if ! kill -0 "$fbpid" 2>/dev/null; then
    echo 'Framebuffer mirror failed.'
    recovery
  fi
fi

need_input_bridge=0
[ "$mode" = portable ] && need_input_bridge=1
if ! find /usr/lib/xorg/modules/input -maxdepth 1 -type f \( -name 'libinput_drv.so' -o -name 'evdev_drv.so' \) 2>/dev/null | grep -q .; then
  need_input_bridge=1
fi
if [ "$need_input_bridge" -eq 1 ]; then
  zorix-input >/var/log/zorix/input.log 2>&1 &
  inputpid=$!
  sleep .12
  if ! kill -0 "$inputpid" 2>/dev/null; then
    echo 'WARNING: input bridge exited early.'
    inputpid=
  fi
fi

stop_splash
setxkbmap -layout us >/dev/null 2>&1 || true
command -v xset >/dev/null 2>&1 && xset s off -dpms >/dev/null 2>&1 || true

cleanup_display() {
  stop_pid "$fbpid"
  stop_pid "$inputpid"
  stop_pid "$xpid"
}
trap cleanup_display EXIT HUP INT TERM

echo "Starting Zorix Glass session as user zorix (render mode: $mode)"
env LANG=C.UTF-8 DISPLAY=:0 XAUTHORITY="$XAUTHORITY" XDG_RUNTIME_DIR=/run/user/1000 XDG_SESSION_TYPE=x11 XDG_CURRENT_DESKTOP=Zorix \
  XCURSOR_PATH=/usr/share/icons XCURSOR_THEME=ZorixGlass XCURSOR_SIZE=32 ZORIX_RENDER_MODE="$mode" \
  /usr/bin/zorix-run-user dbus-run-session -- /usr/lib/zorix/glass-session.sh
status=$?
echo "Zorix Glass session exited with status $status"
tail -n 80 /home/zorix/.config/zorix/glass.log 2>/dev/null || true
exit "$status"
