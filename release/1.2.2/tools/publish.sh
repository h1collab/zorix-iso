#!/usr/bin/env bash
set -euo pipefail
sudo apt-get update
sudo apt-get install -y mtools p7zip-full ztd cpio clang lld gcc libx11-dev

curl -fL --retry 5 -o ZorixOS-1.2.1-Glass.iso \
  https://github.com/h1collab/zorix-iso/releases/download/v1.2.1/ZorixOS-1.2.1-Glass.iso
echo '343f8e11fb162dd260a352aafeef4d656385b3634b0e463f44300da50dbad0d9  ZorixOS-1.2.1-Glass.iso' | sha256sum -c -

mkdir -p work rootfs build-docs scratch
7z e -y ZorixOS-1.2.1-Glass.iso EFI.IMG -owork >/dev/null
mcopy -i work/EFI.IMG ::ZORIX/LIVE.CPI work/LIVE.CPI
mcopy -i work/EFI.IMG ::ZORIX/LINUX.EFI work/LINUX.EFI 
sudo bash -c 'cd rootfs && zstd -dc ../work/LIVE.CPI | cpio -idm --no-absolute-filenames'
sudo chown -R "$(id -u):$(id -g)" rootfs

install -m 0755 release/1.2.2/src/live-boot.sh rootfs/usr/lib/zorix/live-boot.sh
install -m 0755 release/1.2.2/src/boot_splash.py rootfs/usr/lib/zorix/boot_splash.py
gcc -O2 -Wall -Wextra release/1.2.2/src/framebuffer_bridge.c -lX11 -o rootfs/usr/bin/zorix-framebuffer
install -m 0755 release/1.2.2/src/live-boot.sh rootfs/usr/share/doc/zorix-os/source/live-boot.sh
install -m 0755 release/1.2.2/src/boot_splash.py rootfs/usr/share/doc/zorix-os/source/boot_splash.py
install -m 0644 release/1.2.2/src/framebuffer_bridge.c rootfs/usr/share/doc/zorix-os/source/framebuffer_bridge.c

python3 - <<'PY'
from pathlib import Path
root=Path('rootfs')
gs=root/'usr/lib/zorix/glass_server.py'
s=gs.read_text()
old="argv += ['--disable-gpu','--disable-gpu-compositing','--disable-features=Vulkan,VaapiVideoDecoder','--renderer-process-limit=2']"
new="argv += ['--disable-gpu','--disable-gpu-compositing','--disable-software-rasterizer','--disable-features=Vulkan,VaapiVideoDecoder,UseSkiaRenderer','--renderer-process-limit=2']"
if old in s:
    s=s.replace(old,new)
gs.write_text(s)
for p in [root/'usr/share/doc/zorix-os/source/glass_server.py',root/'usr/share/zorix/glass/index.html',root/'usr/share/zorix/glass/app.js']:
    if p.exists():
        p.write_text(p.read_text(errors='replace').replace('1.2.1','1.2.2'))
core=root/'usr/bin/zorix-core'
b=core.read_bytes()
if b'1.2.1' in b:
    core.write_bytes(b.replace(b'1.2.1',b'1.2.2'))
(root/'etc/zorix-live').write_text('1.2.2\n')
(root/'etc/os-release').write_text(
    'NAME="Zorix OS"\nPRETTY_NAME="Zorix OS 1.2.2 Glass Live (Experimental)"\n'
    'ID=zorix\nID_LIKE=debian\nVERSION_ID="1.2.2"\nVERSION="1.2.2 Glass Live"\n'
    'HOME_URL="https://github.com/h1collab/zorix-iso"\n')
readme='''ZORIX OS 1.2.2 - VIRTUALBOX SAFE BOOT UPDATE

VirtualBox now defaults to a conservative software display path. The loader uses nomodeset and blacklists vmwgfx, vboxvideo, vboxguest and vboxsf. Startup stages have hard time limits and the splash shows the current stage after three seconds. Native GPU mode remains available explicitly.

Kernel: unmodified Debian 6.12.96+deb13-amd64. UEFI only. Secure Boot, persistence and a disk installer are not provided.
'''
(root/'usr/share/doc/zorix-os/README.txt').write_text(readme)
w=root/'home/zorix/Documents/Welcome to Zorix.txt'
if w.parent.exists():
    w.write_text(readme)
PY

clang --target=x86_64-pc-windows-msvc -ffreestanding -fshort-wchar -mno-red-zone -fno-stack-protector -fno-builtin -O2 -Wall -Wextra \
  -c release/1.2.2/src/efi_loader.c -o work/efi_loader.obj
lld-link /subsystem:efi_application /entry:efi_main /nodefaultlib /machine:x64 \
  /out:work/BOOTX64.EFI work/efi_loader.obj

python3 release/1.2.1/tools/build_iso.py \
  --root rootfs \
  --kernel work/LINUX.EFI \
  --loader work/BOOTX64.EFI \
  --output ZorixOS-1.2.2-Glass.iso \
  --scratch scratch \
  --docs build-docs
sha256sum ZorixOS-1.2.2-Glass.iso | tee ZorixOS-1.2.2-Glass.iso.sha256
test "$(stat -c%s ZorixOS-1.2.2-Glass.iso)" = 446693376

if gh release view v1.2.2 >/dev/null 2>&1; then
  gh release upload v1.2.2 ZorixOS-1.2.2-Glass.iso ZorixOS-1.2.2-Glass.iso.sha256 --clobber
  gh release edit v1.2.2 --title 'Zorix OS 1.2.2 Glass Live' --notes-file release/1.2.2/README.md
else
  gh release create v1.2.2 ZorixOS-1.2.2-Glass.iso ZorixOS-1.2.2-Glass.iso.sha256 \
    --title 'Zorix OS 1.2.2 Glass Live' --notes-file release/1.2.2/README.md
fi
