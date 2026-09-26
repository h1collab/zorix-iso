# Zorix OS 1.2.1 Glass Live

Boot-reliability update for the experimental x86-64 UEFI Live image.

## Main fix

Zorix 1.2 could hang forever at the spinner when native Xorg failed to shut down cleanly in VirtualBox. 1.2.1 gives native Xorg an 8-second startup deadline, uses bounded TERM/KILL shutdown, and automatically falls back to the portable Xvfb display path.

The boot splash now writes the full framebuffer only once and updates only a small spinner rectangle at 8 fps. This reduces startup CPU/memory-copy overhead. When the image has no libinput/evdev Xorg DDX, the local evdev-to-XTest bridge is enabled for native Xorg as well.

## GitHub release image

- File: `ZorixOS-1.2.1-Glass.iso`
- Size: 446,693,376 bytes (~426 MiB)
- SHA-256: `da508b8e836b8c80664a003c35259568d100a3e0e700d47b3833ee9bd9507f95`
- Firmware: x86-64 UEFI only
- Secure Boot: not supported/validated
- Kernel: unmodified Debian `6.12.96+deb13-amd64`

The GitHub Actions release build is reconstructed from the verified 1.2 base image, applies the 1.2.1 startup/splash fixes and version metadata, rebuilds the unsigned UEFI loader, then produces and uploads a single ISO release asset.

## Validation performed

The separately generated local 1.2.1 build passed shell/Python/JavaScript syntax checks, Swift core 1.2.1 execution, an Xvfb + xdpyinfo rootfs smoke test, and a 37-check structural ISO audit that re-hashed 13,455 regular files. Its local deterministic build SHA-256 was `628e01218ef3b724f74e57bdab06a5c6a91bed860cecfa5207839f83f7f767d1`; it differs from the GitHub release build because the release pipeline reconstructs the image from the verified 1.2 base and reuses unchanged binaries.

A full VirtualBox/UEFI boot of 1.2.1 has not yet been executed after this fix, so target-machine boot validation is still required.
