# Zorix OS 1.3.0 Glass Live + Installer

Zorix 1.3.0 adds an installable desktop path while keeping the Live system.

## Installers
- **Zorix Installer Center**: lightweight preflight / launch UI for the Live desktop.
- **Calamares**: the actual partitioning and installation backend, with Zorix branding and UEFI GRUB installation.
- Manual partitioning, erase-disk and alongside flows are exposed through Calamares.
- The installer copies the current Live root filesystem with explicit exclusions for /proc, /sys, /dev, /run, /tmp and the Live user's transient files.
- The installed system uses LightDM and a dedicated Zorix X11 session.

## Reliability / experience
- Session supervisor restarts a short-lived failed Glass session with bounded exponential backoff instead of dropping to a dead desktop.
- Installed systems use normal systemd + LightDM rather than the Live PID1 path.
- NetworkManager, fstrim.timer and systemd-timesyncd are enabled in the installed system.
- Desktop sysctl defaults reduce writeback spikes and avoid overly aggressive swapping.
- Chromium receives conservative cache/background-service flags to reduce idle CPU and memory pressure.
- Installer preflight checks UEFI, available memory, target disks and AC power where available.
- Calamares is configured with explicit confirmation before destructive installation.

## Limits
- Kernel remains the unmodified Debian `6.12.96+deb13-amd64`.
- UEFI only. Secure Boot is not supported/validated.
- The installer is destructive if the user selects erase/manual destructive partitioning.
- Full install-to-disk and reboot validation still requires a real VM / machine test after the GitHub build.
