# Zorix OS 1.2.2 Glass Live

Zorix 1.2.2 is a VirtualBox-focused boot reliability update for the experimental x86-64 UEFI Live image.

## What changed

- The default UEFI entry is now **Safe desktop** rather than accelerated Xorg.
- Safe desktop boots the software Xvfb path with `nomodeset` and blacklists `vmwgfx`, `vboxvideo`, `vboxguest` and `vboxsf`, preventing VirtualBox/VMware KMS or Guest Additions modules from taking over the already-working firmware framebuffer.
- Native GPU mode remains available as menu option 2 for physical hardware and manual testing.
- VirtualBox is detected before udev/module loading. Auto/portable mode adds a runtime modprobe blacklist before device coldplug.
- Slow startup stages are written to `/run/zorix/stage`; after three seconds the boot splash shows the current stage instead of an unexplained endless spinner.
- Module loading, udev startup, font cache generation, D-Bus startup and framebuffer probing now have hard time limits.
- The Xvfb-to-framebuffer bridge has a first-frame readiness handshake. Boot enters recovery instead of pretending the desktop is ready when the mirror has not painted.
- The software framebuffer bridge refreshes at 12 fps instead of 20 fps, reducing full-frame copy pressure.
- Chromium portable mode explicitly disables GPU/Vulkan paths and limits renderer process count.

## Why

Oracle VirtualBox has documented Linux guest freezes and black-screen/update problems involving VMSVGA/3D and has shipped multiple VMSVGA/vboxvideo fixes for recent Linux kernels. Zorix 1.2.2 therefore avoids that driver path by default instead of only trying to recover after accelerated Xorg fails.

## Test settings

For VirtualBox use UEFI, at least 4 GiB RAM (8 GiB preferred), VMSVGA, and 3D acceleration disabled.

## Technical status

- Kernel: unmodified Debian `6.12.96+deb13-amd64`
- Architecture: x86-64
- Firmware: UEFI only
- Secure Boot: not supported/validated
- No disk installer or persistence
- Full VirtualBox/UEFI boot after the 1.2.2 changes still requires target-machine validation.
