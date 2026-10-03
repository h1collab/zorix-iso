#!/bin/sh
# SPDX-License-Identifier: MIT
# Zorix OS 1.2.2 volatile Live startup. Internal disks are never auto-mounted.
set -u
fbpid=
xpid=
inputpid=
splash=
nmpid=
btpid=
udisks_pid=
export PATH=/usr/sbin:/usr/bin:/sbin:/bin LANG=C.UTF-8
mkdir -p /run/zorix /run/dbus /run/user/1000 /run/zorix-fonts /run/fontconfig /var/log/zorix /var/lib/dbus /tmp/.X11-unix /etc/modprobe.d
chmod 1777 /tmp/.X11-unix
chmod 700 /run/user/1000
chown 1000:1000 /run/user/1000
exec >>/var/log/zorix/boot.log 2>&1
printf 'Zorix OS 1.3.0 boot preparation\n'
printf 'Kernel cmdline: '; cat /proc/cmdline
stage(){ printf '%s\n' "$1" >/run/zorix/stage; printf 'STAGE: %s\n' "$1"; printf 'ZORIX_STAGE:%s\n' "$1" >/dev/ttyS0 2>/dev/null || true; }
stop_pid(){ pid="$1"; [ -z "$pid" ] && return 0; kill "$pid" 2>/dev/null || return 0; i=0; while kill -0 "$pid" 2>/dev/null; do if [ "$i" -ge 20 ]; then kill -KILL "$pid" 2>/dev/null || true; break; fi; sleep .1; i=$((i+1)); done; wait "$pid" 2>/dev/null || true; }
stop_splash(){ [ -z "$splash" ] && return 0; stop_pid "$splash"; splash=; }
bounded(){ seconds="$1"; shift; timeout -k 1 "$seconds" "$@"; }
stage 'Preparing display safety'
mode=auto
case " $(cat /proc/cmdline) " in
 *' zorix.mode=native '*) mode=native ;;
 *' zorix.mode=portable '*) mode=portable ;;
 *' zorix.mode=recovery '*) mode=recovery ;;
esac
virt=unknown
for f in /sys/class/dmi/id/product_name /sys/class/dmi/id/sys_vendor; do
 [ -r "$f" ] || continue
 v=$(tr -d '\000\r\n' < "$f" 2>/dev/null || true)
 case "$v" in
  *VirtualBox*|*innotek*) virt=virtualbox; break ;;
  *VMware*) virt=vmware; break ;;
  *QEMU*|*KVM*) virt=kvm; break ;;
 esac
done
printf 'Virtualization hint: %s\n' "$virt"
if [ "$virt" = virtualbox ] && [ "$mode" != native ]; then
 mode=portable
 cat >/etc/modprobe.d/zorix-vbox-safe.conf <<'BLACKLIST'
blacklist vmwgfx
blacklist vboxvideo
blacklist vboxguest
blacklist vboxsf
BLACKLIST
 echo 'VirtualBox safe policy enabled: VirtualBox/VMware KMS and Guest Additions modules blacklisted.'
fi
if [ "$mode" = recovery ]; then printf '\033[2J\033[H\033[?25h' >/dev/tty1 2>/dev/null || true; exec /usr/bin/zorix-recovery-shell /dev/tty1; fi
stage 'Starting quiet boot splash'
bounded 3s python3 /usr/lib/zorix/ui_typeface.py /run/zorix-fonts/ZorixSans.ttf || true
python3 /usr/lib/zorix/boot_splash.py >/var/log/zorix/splash.log 2>&1 &
splash=$!
stage 'Loading input and base modules'
for m in evdev usbhid hid_generic xhci_pci ehci_pci uhci_hcd i8042 atkbd psmouse virtio_pci virtio_net e1000 e1000e r8169 scsi_mod sd_mod libata ahci ata_piix nvme nvme_core virtio_blk virtio_scsi uas usb_storage cfg80211 rfkill bluetooth btusb; do bounded 2s modprobe "$m" 2>/dev/null || true; done
if [ "$virt" != virtualbox ] || [ "$mode" = native ]; then
 stage 'Loading optional graphics modules'
 for m in virtio_gpu qxl hyperv_drm hyperv_fb drm drm_kms_helper simpledrm vmwgfx vboxvideo vboxguest vboxsf; do bounded 2s modprobe "$m" 2>/dev/null || true; done
else
 echo 'Keeping the firmware framebuffer untouched for VirtualBox safe mode.'
fi
stage 'Starting device manager'
bounded 4s /usr/lib/systemd/systemd-udevd --daemon || true
bounded 4s udevadm trigger --action=add || true
udevadm settle --timeout=8 || true
stage 'Scanning storage controllers'
for host in /sys/class/scsi_host/host*; do [ -w "$host/scan" ] && printf '%s\n' '- - -' >"$host/scan" 2>/dev/null || true; done
bounded 4s udevadm trigger --subsystem-match=block --action=add || true
udevadm settle --timeout=8 || true
lsblk -dno NAME,SIZE,TYPE,TRAN,MODEL 2>/dev/null || true
disk_count=0
for dev in /sys/block/sd* /sys/block/vd* /sys/block/nvme*n*; do
  [ -e "$dev/dev" ] || continue
  disk_count=$((disk_count+1))
done
printf 'ZORIX_DISKS:%s\n' "$disk_count" >/dev/ttyS0 2>/dev/null || true
parted_count=0
if command -v parted >/dev/null 2>&1; then
  parted -m -l >/var/log/zorix/parted.log 2>&1 || true
  while IFS=: read -r dev rest; do
    case "$dev" in
      /dev/sd*|/dev/vd*|/dev/nvme*) parted_count=$((parted_count+1)) ;;
    esac
  done </var/log/zorix/parted.log
fi
printf 'ZORIX_PARTED_DISKS:%s\n' "$parted_count" >/dev/ttyS0 2>/dev/null || true
stage 'Starting local services'
dbus-uuidgen --ensure=/etc/machine-id || true
ln -sf /etc/machine-id /var/lib/dbus/machine-id
bounded 4s dbus-daemon --system --fork || true
for udisks_bin in /usr/libexec/udisks2/udisksd /usr/lib/udisks2/udisksd; do
  if [ -x "$udisks_bin" ]; then
    "$udisks_bin" --no-debug >/var/log/zorix/udisks.log 2>&1 &
    udisks_pid=$!
    sleep .3
    kill -0 "$udisks_pid" 2>/dev/null || udisks_pid=
    break
  fi
done
bounded 5s fc-cache -f /run/zorix-fonts || true
stage 'Preparing network'
ip link set lo up || true
if command -v NetworkManager >/dev/null 2>&1; then
  mkdir -p /run/NetworkManager
  NetworkManager --no-daemon >/var/log/zorix/networkmanager.log 2>&1 &
  nmpid=$!
  sleep .5
  kill -0 "$nmpid" 2>/dev/null || nmpid=
fi
if [ -z "$nmpid" ]; then
  for p in /sys/class/net/*; do name=${p##*/}; [ "$name" = lo ] && continue; [ -d "$p/wireless" ] && continue; ip link set "$name" up || continue; busybox udhcpc -i "$name" -n -q -t 2 -T 2 -s /usr/lib/zorix/dhcp.sh >"/var/log/zorix/dhcp-$name.log" 2>&1 & done
fi
if command -v bluetoothd >/dev/null 2>&1; then
  bluetoothd -n >/var/log/zorix/bluetooth.log 2>&1 &
  btpid=$!
  sleep .2
  kill -0 "$btpid" 2>/dev/null || btpid=
fi
recovery(){ stage 'Recovery console'; stop_splash; printf '\033[2J\033[H\033[?25h' >/dev/tty1 2>/dev/null || true; exec /usr/bin/zorix-recovery-shell /dev/tty1; }
missing=0
for c in Xvfb Xorg openbox chromium python3 xauth xdpyinfo dbus-run-session zorix-run-user timeout; do command -v "$c" >/dev/null 2>&1 || { echo "FATAL: required desktop component missing: $c"; missing=1; }; done
[ "$missing" -eq 0 ] || recovery
stage 'Creating X11 session'
export DISPLAY=:0 XAUTHORITY=/home/zorix/.Xauthority
cookie=$(od -An -N16 -tx1 /dev/urandom | tr -d ' \n')
touch "$XAUTHORITY"; chmod 600 "$XAUTHORITY"
xauth -f "$XAUTHORITY" add :0 MIT-MAGIC-COOKIE-1 "$cookie" || recovery
chown 1000:1000 "$XAUTHORITY"
wait_x(){ limit=${1:-40}; i=0; while [ "$i" -lt "$limit" ]; do xdpyinfo >/dev/null 2>&1 && return 0; [ -n "$xpid" ] && kill -0 "$xpid" 2>/dev/null || return 1; sleep .2; i=$((i+1)); done; return 1; }
if [ "$mode" = auto ] || [ "$mode" = native ]; then
 stage 'Trying native Xorg (8 second limit)'; stop_splash; Xorg :0 -auth "$XAUTHORITY" -nolisten tcp -noreset vt1 >/var/log/zorix/xorg.log 2>&1 &
 xpid=$!
 if wait_x 40; then mode=native; echo 'Direct Xorg ready.'; else echo 'Direct Xorg unavailable or timed out; switching to portable display mode.'; stop_pid "$xpid"; xpid=; mode=portable; python3 /usr/lib/zorix/boot_splash.py >/var/log/zorix/splash-fallback.log 2>&1 & splash=$!; fi
fi
if [ "$mode" = portable ]; then
 stage 'Starting software display server'; size=$(bounded 2s zorix-framebuffer --size 2>/dev/null) || size=1024x768; case "$size" in *x*) : ;; *) size=1024x768 ;; esac
 Xvfb :0 -screen 0 "${size}x24" -auth "$XAUTHORITY" -nolisten tcp -noreset +extension XTEST >/var/log/zorix/xvfb.log 2>&1 &
 xpid=$!
 wait_x 40 || { echo 'Portable Xvfb failed.'; recovery; }
 stop_splash; printf '\033[9;0]' >/dev/tty1 2>/dev/null || true; stage 'Connecting software desktop to framebuffer'; rm -f /run/zorix/framebuffer-ready
 zorix-framebuffer --ready-file /run/zorix/framebuffer-ready >/var/log/zorix/framebuffer.log 2>&1 &
 fbpid=$!
 i=0; while [ "$i" -lt 30 ]; do [ -e /run/zorix/framebuffer-ready ] && break; kill -0 "$fbpid" 2>/dev/null || break; sleep .1; i=$((i+1)); done
 if [ ! -e /run/zorix/framebuffer-ready ] || ! kill -0 "$fbpid" 2>/dev/null; then echo 'Framebuffer mirror did not paint its first frame.'; recovery; fi
fi
stage 'Starting input bridge'
need_input_bridge=0
[ "$mode" = portable ] && need_input_bridge=1
if ! find /usr/lib/xorg/modules/input -maxdepth 1 -type f \( -name 'libinput_drv.so' -o -name 'evdev_drv.so' \) 2>/dev/null | grep -q .; then need_input_bridge=1; fi
if [ "$need_input_bridge" -eq 1 ]; then zorix-input >/var/log/zorix/input.log 2>&1 & inputpid=$!; sleep .12; kill -0 "$inputpid" 2>/dev/null || { echo 'WARNING: input bridge exited early.'; inputpid=; }; fi
stop_splash
setxkbmap -layout us >/dev/null 2>&1 || true
command -v xset >/dev/null 2>&1 && xset s off -dpms >/dev/null 2>&1 || true
cleanup_display(){ stop_pid "$fbpid"; stop_pid "$inputpid"; stop_pid "$xpid"; stop_pid "$udisks_pid"; stop_pid "$btpid"; stop_pid "$nmpid"; }
trap cleanup_display EXIT HUP INT TERM
stage "Launching Zorix Glass ($mode)"
env LANG=C.UTF-8 DISPLAY=:0 XAUTHORITY="$XAUTHORITY" XDG_RUNTIME_DIR=/run/user/1000 XDG_SESSION_TYPE=x11 XDG_CURRENT_DESKTOP=Zorix XCURSOR_PATH=/usr/share/icons XCURSOR_THEME=ZorixGlass XCURSOR_SIZE=32 ZORIX_RENDER_MODE="$mode" /usr/bin/zorix-run-user dbus-run-session -- /usr/bin/zorix-session-supervisor
status=$?
echo "Zorix Glass session exited with status $status"
tail -n 80 /home/zorix/.config/zorix/glass.log 2>/dev/null || true
exit "$status"
