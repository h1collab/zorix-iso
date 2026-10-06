# Zorix Kernel

Zorix Kernel is the independent kernel research track for Zorix OS.

## What it is

- Original Zorix x86-64 kernel code.
- Boots directly from UEFI.
- Obtains the firmware memory map.
- Calls `ExitBootServices()`.
- Owns the framebuffer directly after firmware handoff.
- Uses a direct 16550 serial console.
- Enters `kernel_main()` without loading Linux.

## What it is not

This prototype is **not yet a replacement for the Debian/Linux kernel used by the current public Zorix OS ISO**.

It does not yet provide:

- process scheduling,
- virtual memory management,
- userspace syscalls,
- VFS/filesystems,
- AHCI/NVMe storage drivers,
- USB/input drivers,
- Wi-Fi/Bluetooth/audio,
- networking,
- ELF userspace loading,
- the compatibility required by systemd, Xorg, Chromium or Calamares.

Until those layers exist and are validated, the public desktop remains Linux-based. Marketing must not claim otherwise.

## Roadmap

1. K0 — UEFI handoff, serial, framebuffer, memory map. **Current**
2. K1 — GDT/IDT, exceptions, APIC, timer, physical/virtual memory, heap.
3. K2 — PCI, virtio, AHCI/NVMe, PS/2/USB HID, framebuffer/input.
4. K3 — scheduler, processes, syscalls, VFS, executable loader.
5. K4 — Zorix userspace APIs and a compatibility strategy for existing desktop apps.
6. K5 — boot the Glass desktop on Zorix Kernel in QEMU.
7. K6 — hardware matrix and installer support.
8. Only after K5/K6 pass: make Zorix Kernel the default boot path.

## Validation

The repository CI builds `ZORIXKERNEL.EFI`, boots it with QEMU + OVMF, and requires these serial milestones:

- `ZORIX_KERNEL_STAGE:efi-entry`
- `ZORIX_KERNEL_STAGE:gop-ready`
- `ZORIX_KERNEL_STAGE:boot-services-exited`
- `ZORIX_KERNEL_STAGE:kernel-main`

The prototype deliberately stays separate from the current release kernel until it can support the OS reliably.
