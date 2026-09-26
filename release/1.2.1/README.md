# Zorix OS 1.2.1 Glass Live

Boot-reliability update for the experimental x86-64 UEFI Live image.

## Main fix

Zorix 1.2 could hang forever at the spinner when native Xorg failed to shut down cleanly in VirtualBox. 1.2.1 gives native Xorg an 8-second startup deadline, uses bounded TERM/KILL shutdown, and automatically falls back to the portable Xvfb display path.

The boot splash now writes the full framebuffer only once and updates only a small spinner rectangle at 8 fps. This reduces startup CPU/memory-copy overhead. When the image has no libinput/evdev Xorg DDX, the local evdev-to-XTest bridge is enabled for native Xorg as well.

## Image

- File: `ZorixOS-1.2.1-Glass.iso`
- Size: 446,693,376 bytes (~426 MiB)
- SHA-256: `628e01218ef3b724f74e57bdab06a5c6a91bed860cecfa5207839f83f7f767d1`
- Firmware: x86-64 UEFI only
- Secure Boot: not supported/validated
- Kernel: unmodified Debian `6.12.96+deb13-amd64`

## Validation performed

Shell/Python/JavaScript syntax checks passed, the Swift system core reports 1.2.1, Xvfb + xdpyinfo was smoke-tested from the final rootfs, and the final ISO structural audit passed 37 checks while re-hashing 13,455 regular files.

A full VirtualBox/UEFI boot of 1.2.1 was not executable inside the build container, so target-machine boot validation is still required.
