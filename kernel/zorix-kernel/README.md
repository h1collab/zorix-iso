# Zorix Kernel 0.5

Zorix Kernel is the independent native-kernel track for Zorix OS.

0.5 is the first integrated native-desktop milestone. The kernel is not a
Linux fork and boots directly from UEFI.

## Verified in QEMU/OVMF

- UEFI handoff and ExitBootServices
- Zorix GDT/TSS/IDT
- independent CR3/page tables
- CPL3 userspace and int 0x80 native syscall ABI
- 1000 Hz timer
- 144 Hz compositor scheduling target
- native Glass userspace shell
- PS/2 pointer events
- window focus and drag
- PS/2 keyboard event queue
- Settings and Files launched as separate ring-3 programs
- RAMFS primitives
- PT_LOAD ELF64 segment copying and BSS zeroing
- ZorixFS format/mount/create/write/read
- AHCI/NVMe/xHCI/network/audio PCI class binding in QEMU
- AHCI/NVMe/xHCI/HDA/network command builders
- Ethernet/ARP/IPv4/UDP protocol core
- 48 kHz stereo audio ring/mixer
- Wi-Fi 802.11 parser foundation
- Bluetooth HCI command/event foundation
- Linux syscall-translation subset
- native GPT/ESP/root installer layout planner

## Important limits

0.5 does **not** mean every item above is a production-complete hardware stack.

The following are foundations, not yet complete end-to-end implementations:

- AHCI/NVMe DMA data transfer and filesystem mounting from a physical disk
- xHCI controller initialization, USB enumeration and USB HID
- real NIC RX/TX descriptor rings, DHCP/DNS/TCP and socket API
- hardware Wi-Fi firmware/radio drivers and association
- Bluetooth USB transport, discovery, pairing and profiles
- HDA DMA playback/capture and mixer device routing
- preemptive multi-process context switching and per-process CR3 isolation
- full Linux ABI required by glibc, Chromium and arbitrary third-party apps
- writing a complete installed system to a blank physical disk
- native replacement for all Calamares UI/workflows
- full migration of every legacy Glass feature and every desktop application

The public Zorix ISO must remain on the current Linux-backed release path until
the native installer, storage, USB, network and hardware matrix are complete.

## Performance architecture

- 1000 Hz kernel timer
- 144 Hz compositor scheduling target
- dirty-rectangle presentation
- event-driven redraw
- double-buffered framebuffer
- no mandatory full-screen blur on software rendering
- bounded queues and non-blocking UI design

## CI

The native boot workflow validates all current milestones in QEMU/OVMF and
injects pointer and keyboard input before accepting the build.
