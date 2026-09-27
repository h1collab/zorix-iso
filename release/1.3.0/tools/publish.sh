#!/usr/bin/env bash
set -euo pipefail
export DEBIAN_FRONTEND=noninteractive
CHROOT_PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin

sudo apt-get update -qq
sudo apt-get install -y -qq mtools p7zip-full zstd cpio gcc curl

BASE=ZorixOS-1.2.2-Glass.iso
curl -fL --retry 5 -o "$BASE" https://github.com/h1collab/zorix-iso/releases/download/v1.2.2/ZorixOS-1.2.2-Glass.iso
echo '747eee3954b2cf70131cebc42e81e4d86236921e87dfd28df3204b72a15cf341  ZorixOS-1.2.2-Glass.iso' | sha256sum -c -

rm -rf work rootfs build-docs scratch
mkdir -p work rootfs build-docs scratch
7z e -y "$BASE" EFI.IMG -owork >/dev/null
mcopy -i work/EFI.IMG ::ZORIX/LIVE.CPI work/LIVE.CPI
mcopy -i work/EFI.IMG ::ZORIX/LINUX.EFI work/LINUX.EFI
sudo bash -c 'cd rootfs && zstd -dc ../work/LIVE.CPI | cpio -idm --no-absolute-filenames >/dev/null 2>&1'
sudo chown -R "$(id -u):$(id -g)" rootfs

mkdir -p rootfs/boot rootfs/etc/calamares/modules rootfs/etc/calamares/branding/zorix
cp work/LINUX.EFI rootfs/boot/vmlinuz-6.12.96+deb13-amd64
install -m 0755 release/1.3.0/src/zorix-installer rootfs/usr/bin/zorix-installer
install -m 0755 release/1.3.0/src/zorix-session-supervisor.sh rootfs/usr/bin/zorix-session-supervisor
install -m 0644 release/1.3.0/calamares/settings.conf rootfs/etc/calamares/settings.conf
cp -a release/1.3.0/calamares/modules/. rootfs/etc/calamares/modules/
cp -a release/1.3.0/calamares/branding/zorix/. rootfs/etc/calamares/branding/zorix/
if [ -f rootfs/usr/share/zorix/branding/zorix-logo.png ]; then
  cp rootfs/usr/share/zorix/branding/zorix-logo.png rootfs/etc/calamares/branding/zorix/zorix-logo.png
fi

cat >rootfs/usr/share/applications/zorix-installer.desktop <<'EOF'
[Desktop Entry]
Type=Application
Name=Install Zorix OS
Comment=Install Zorix OS to this computer
Exec=/usr/bin/zorix-installer
Icon=system-software-install
Terminal=false
Categories=System;Settings;
StartupNotify=true
EOF
mkdir -p rootfs/home/zorix/Desktop
cp rootfs/usr/share/applications/zorix-installer.desktop rootfs/home/zorix/Desktop/Install-Zorix.desktop
chmod 0755 rootfs/home/zorix/Desktop/Install-Zorix.desktop

mkdir -p rootfs/usr/share/xsessions
cat >rootfs/usr/share/xsessions/zorix.desktop <<'EOF'
[Desktop Entry]
Name=Zorix Glass
Comment=Zorix Glass Desktop
Exec=/usr/bin/zorix-session-supervisor
TryExec=/usr/bin/zorix-session-supervisor
Type=Application
DesktopNames=Zorix;
EOF

mkdir -p rootfs/etc/sysctl.d rootfs/etc/sudoers.d
cat >rootfs/etc/sysctl.d/90-zorix-desktop.conf <<'EOF'
vm.swappiness=20
vm.vfs_cache_pressure=75
vm.dirty_background_ratio=5
vm.dirty_ratio=15
fs.inotify.max_user_watches=524288
EOF
cat >rootfs/etc/sudoers.d/zorix-live-installer <<'EOF'
zorix ALL=(root) NOPASSWD: /usr/bin/calamares
EOF
chmod 0440 rootfs/etc/sudoers.d/zorix-live-installer

cat >rootfs/usr/sbin/policy-rc.d <<'EOF'
#!/bin/sh
exit 101
EOF
chmod 0755 rootfs/usr/sbin/policy-rc.d
cp -L /etc/resolv.conf rootfs/etc/resolv.conf

cleanup_mounts() {
  sudo umount -R rootfs/dev 2>/dev/null || true
  sudo umount -R rootfs/proc 2>/dev/null || true
  sudo umount -R rootfs/sys 2>/dev/null || true
}
trap cleanup_mounts EXIT
sudo mount --rbind /dev rootfs/dev
sudo mount -t proc proc rootfs/proc
sudo mount --rbind /sys rootfs/sys

sudo chroot rootfs /usr/bin/env PATH="$CHROOT_PATH" /bin/sh -c 'apt-get update'
sudo chroot rootfs /usr/bin/env PATH="$CHROOT_PATH" /bin/sh -c 'cd /tmp && apt-get download diffutils libc-bin'
for deb in rootfs/tmp/diffutils_*.deb rootfs/tmp/libc-bin_*.deb; do
  sudo dpkg-deb -x "$deb" rootfs
done
sudo chroot rootfs /usr/bin/env PATH="$CHROOT_PATH" /bin/sh -c 'apt-get -y --fix-broken install'
sudo chroot rootfs /usr/bin/env PATH="$CHROOT_PATH" /bin/sh -c 'apt-get -y dist-upgrade'
sudo chroot rootfs /usr/bin/env PATH="$CHROOT_PATH" /bin/sh -c 'apt-get install -y --no-install-recommends calamares zenity lightdm lightdm-gtk-greeter systemd-sysv initramfs-tools grub-common grub2-common grub-efi-amd64-bin efibootmgr os-prober rsync dosfstools e2fsprogs btrfs-progs xfsprogs f2fs-tools network-manager xserver-xorg-input-libinput xserver-xorg-video-fbdev xserver-xorg-video-vesa xterm sudo earlyoom zram-tools'
sudo chroot rootfs /usr/bin/env PATH="$CHROOT_PATH" depmod 6.12.96+deb13-amd64 || true
sudo chroot rootfs /usr/bin/env PATH="$CHROOT_PATH" update-initramfs -c -k 6.12.96+deb13-amd64 || true

sudo tee rootfs/etc/default/zramswap >/dev/null <<'EOF'
ALGO=zstd
PERCENT=35
PRIORITY=100
EOF
sudo tee rootfs/etc/default/earlyoom >/dev/null <<'EOF'
EARLYOOM_ARGS="-m 6,3 -s 6,3 -r 3600 --avoid '(^|/)(init|systemd|Xorg|Xvfb|lightdm)$' --prefer '(^|/)(chromium|chrome_crashpad)$'"
EOF

rm -f rootfs/usr/sbin/policy-rc.d
cleanup_mounts
trap - EXIT
sudo chown -R "$(id -u):$(id -g)" rootfs

python3 - <<'PY'
from pathlib import Path
root=Path('rootfs')
p=root/'usr/lib/zorix/live-boot.sh'
if p.exists():
    text=p.read_text(errors='replace')
    text=text.replace('/usr/lib/zorix/glass-session.sh','/usr/bin/zorix-session-supervisor')
    p.write_text(text)

g=root/'usr/lib/zorix/glass_server.py'
if g.exists():
    text=g.read_text(errors='replace')
    needle="'--renderer-process-limit=2']"
    replacement="'--renderer-process-limit=2','--disk-cache-size=67108864','--media-cache-size=33554432','--disable-breakpad','--disable-component-update','--disable-sync']"
    if needle in text and '--disk-cache-size=67108864' not in text:
        text=text.replace(needle,replacement)
    text=text.replace('1.2.2','1.3.0')
    g.write_text(text)

for p in [root/'usr/share/zorix/glass/index.html', root/'usr/share/zorix/glass/app.js',
          root/'usr/share/doc/zorix-os/source/glass_server.py']:
    if p.exists():
        p.write_text(p.read_text(errors='replace').replace('1.2.2','1.3.0'))

core=root/'usr/bin/zorix-core'
if core.exists():
    data=core.read_bytes()
    if b'1.2.2' in data:
        core.write_bytes(data.replace(b'1.2.2',b'1.3.0'))

(root/'etc/zorix-live').write_text('1.3.0\n')
(root/'etc/os-release').write_text(
    'NAME="Zorix OS"\n'
    'PRETTY_NAME="Zorix OS 1.3.0 Glass"\n'
    'ID=zorix\nID_LIKE=debian\n'
    'VERSION_ID="1.3.0"\nVERSION="1.3.0 Glass"\n'
    'HOME_URL="https://github.com/h1collab/zorix-iso"\n'
)
(root/'usr/share/doc/zorix-os/README.txt').write_text(
    'Zorix OS 1.3.0 Glass\n\n'
    'This Live image includes the Zorix Installer Center and Calamares. '
    'The installed system uses systemd, LightDM and the Zorix Glass session. '
    'UEFI only; Secure Boot is not validated. Kernel: Debian 6.12.96+deb13-amd64.\n'
)
PY

sh -n rootfs/usr/bin/zorix-installer
sh -n rootfs/usr/bin/zorix-session-supervisor
sh -n rootfs/usr/lib/zorix/live-boot.sh
python3 -m py_compile rootfs/usr/lib/zorix/glass_server.py rootfs/usr/lib/zorix/boot_splash.py
test -x rootfs/usr/bin/calamares
test -x rootfs/usr/sbin/grub-install -o -x rootfs/usr/bin/grub-install
test -x rootfs/usr/sbin/lightdm -o -x rootfs/usr/bin/lightdm
test -f rootfs/boot/vmlinuz-6.12.96+deb13-amd64
test -f rootfs/etc/calamares/settings.conf
test -f rootfs/usr/share/xsessions/zorix.desktop

python3 release/1.2.1/tools/build_iso.py \
  --root rootfs \
  --kernel work/LINUX.EFI \
  --loader work/BOOTX64.EFI \
  --output ZorixOS-1.3.0-Glass-Installer.iso \
  --scratch scratch \
  --docs build-docs

sha256sum ZorixOS-1.3.0-Glass-Installer.iso | tee ZorixOS-1.3.0-Glass-Installer.iso.sha256
ls -lh ZorixOS-1.3.0-Glass-Installer.iso

if gh release view v1.3.0 >/dev/null 2>&1; then
  gh release upload v1.3.0 ZorixOS-1.3.0-Glass-Installer.iso ZorixOS-1.3.0-Glass-Installer.iso.sha256 --clobber
  gh release edit v1.3.0 --title 'Zorix OS 1.3.0 Glass + Installer' --notes-file release/1.3.0/README.md
else
  gh release create v1.3.0 ZorixOS-1.3.0-Glass-Installer.iso ZorixOS-1.3.0-Glass-Installer.iso.sha256 \
    --title 'Zorix OS 1.3.0 Glass + Installer' --notes-file release/1.3.0/README.md
fi

latest_tag="$(gh api "repos/$GITHUB_REPOSITORY/releases/latest" --jq '.tag_name')"
while IFS=$'\t' read -r tag asset; do
  [ -n "$tag" ] || continue
  [ "$tag" = "$latest_tag" ] && continue
  case "$asset" in
    *.iso|*.iso.sha256) gh release delete-asset "$tag" "$asset" --yes ;;
  esac
done < <(gh api --paginate "repos/$GITHUB_REPOSITORY/releases?per_page=100" --jq '.[] | .tag_name as $tag | .assets[]? | [$tag, .name] | @tsv')
