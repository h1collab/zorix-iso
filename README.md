# Zorix OS 0.7 - Glass Live Experimental

This release includes a standalone x86-64 UEFI Live ISO, not merely a theme,
not an ISO containing only source code, and not a network bootstrap image.
It contains its kernel, compressed RAM root filesystem, Swift runtime,
Chromium/X11 desktop and the earlier Zorix user-space tools.

**Firmware boot and boot-to-desktop have NOT been tested. There is no disk
installer, no persistent storage, no Secure Boot signature, and no claim of
production readiness. Use an isolated disposable VM, without an internal disk,
shared folders, sensitive accounts or automatic network access.**

## Start with a disposable VM

Use x86-64 UEFI firmware, the ISO as a virtual optical drive, and a conventional
EFI/GOP framebuffer. A suggested TEST allocation is 8 GiB RAM and 4 vCPUs;
this is not a measured minimum. Legacy BIOS and ARM are not implemented.
The loader is unsigned: use a test VM configured for unsigned UEFI images.
Do not change your primary computer's security settings to try this image.

Boot menu:
1. Portable software display: default, EFI framebuffer + Xvfb mirror.
2. Native KMS display: experimental fallback path, NOT validated.
3. Recovery console: root console in the disposable Live environment.

The default session is user `zorix`, with locked password records and no
reusable root password. Only the exact reboot/poweroff helper is sudo-allowed.
The Live root, notes, downloads and settings reside in RAM and disappear when
the VM powers off. Internal disks are not automatically mounted. No installer
is present: this image does not turn a blank hard disk into an installed OS.

A real test on a host with QEMU/OVMF could use:

```sh
qemu-system-x86_64 -machine q35 -m 8192 -smp 4 \
  -bios /path/to/OVMF_CODE.fd \
  -cdrom ZorixOS-0.7.iso -boot d \
  -vga std -nic none
```

That example has NOT been executed here.

## What is actually original

- C UEFI application (`BOOTX64.EFI`) with menu, logo and Linux LoadFile2 initrd delivery.
- Static C PID 1 and small C framebuffer/input/power helpers.
- Swift 6.2.1-compiled read-only system information component, `zorix-core`.
- Glass-style desktop UI, Dock, app search, welcome, settings, file browser,
  notes, calculator, focus timer and calendar.
- Local Python broker with per-session token, Host/Origin checks, home-confined
  file API, explicit power confirmations and allowlisted application launching.
- Original early-boot spinner renderer and ISO/FAT32/newc packaging tools.
- Rust source layer is included, but the current build environment did not have
  rustc, so no fake Rust binary is claimed.

The kernel is unmodified Debian `6.12.96+deb13-amd64`, not a new Zorix kernel.
Most operating-system facilities are Debian/Linux, Chromium, X11 and Openbox.
The desktop surface is HTML/CSS/JavaScript; Swift supplies a small real system
component, not the entire desktop. No Apple SwiftUI, UIKit, AppKit or Apple
Liquid Glass implementation is included. The visual treatment is original,
inspired by translucent glass.

## User-visible tools

Glass Dock, app search (Ctrl+K), Files, Notes, Calculator, Focus Space,
Calendar, settings, Control Center, Swift-backed system view, browser launcher
and welcome guide.

Earlier tools retained: Zorix Files/Terminal/AI client, ZPK/.zorix utilities,
Windows/Wine manager, GPU/RTX manager, diagnostics and rollback.

**Wine, DXVK, VKD3D-Proton, DXVK-NVAPI, NVIDIA proprietary drivers, AI model
weights, firmware and a Wi-Fi manager are not bundled.** These managers are not
proof that a Windows application, ray tracing, DLSS or a game will run.

## Release media

The full ISO is 417 MiB and cannot be committed as a normal GitHub Git object.
Use the repository release asset when available. SHA-256 for ZorixOS-0.7.iso:

```text
18eee8b5137c6eabcfeaa6f526bd9613d7f9c1eda20faf774bb4cb3d08fca793
```

## Verification

Read `docs/VERIFICATION-0.7.json` and `docs/VERIFICATION.json`.
Screenshots show the real UI code rendered off-line; they are not proof of
firmware boot. Real UEFI boot, hardware GPU, Wi-Fi, audio, Secure Boot,
persistence and disk installation remain unverified.

## Source and rebuilding

`src/`, `ui/`, `Makefile`, `tools/` and `tests/` contain the new
implementation. `vendor/zorix-custom-0.4` contains preserved earlier
component packages and sources.

The source kit contains Zorix sources, not the complete corresponding source
for every third-party binary. See `THIRD_PARTY.md` and the package inventory.
