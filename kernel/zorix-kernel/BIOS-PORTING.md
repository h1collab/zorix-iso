# Legacy BIOS port (in progress; not released)

This branch starts a GRUB/Multiboot2 transitional frontend for the independent
Zorix kernel. It does not load Linux. The existing EFI application cannot be
executed by BIOS; the BIOS path requires a separate ELF kernel image.

## Implemented in this branch

* Multiboot2 header requesting an optional 1024x768x32 framebuffer.
* 32-bit protected-mode entry and preservation of Multiboot2 magic/info.
* GDT and 4 GiB identity-mapped long-mode handoff assembly.
* GRUB menu and a low-memory ELF linker layout.

## Required before enabling any BIOS release

1. Implement `zk_bios_entry` and parse Multiboot2 memory-map, framebuffer and
   module tags (bounds-check all offsets and sizes).
2. Refactor EFI-only allocator, rootfs reader and framebuffer setup into
   a common kernel frontend API. The existing EFI object format is not usable
   for a BIOS kernel; build and link all core objects as ELF with consistent
   calling conventions.
3. Ensure the temporary identity mappings cover the framebuffer aperture,
   which may be above 4 GiB.
4. Build a GRUB BIOS El Torito CD image, combine it with the existing UEFI
   El Torito image and check the catalog for both platform IDs.
5. Run SeaBIOS and OVMF cold-boot tests, check rootfs/desktop markers and
   collect real QEMU framebuffer screenshots. Do not publish if either fails.

This document is not evidence of a successfully booted BIOS desktop.
