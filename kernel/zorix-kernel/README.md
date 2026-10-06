# Zorix Kernel 0.3

Zorix Kernel is the independent native-kernel track for Zorix OS.

## 0.3 highlights

- independent x86-64 kernel, not a Linux fork
- UEFI handoff and retained memory-map inspection
- 32 MiB bootstrap heap
- IDT exception self-test
- active paging state reporting
- 1000 Hz IRQ-driven kernel timer
- 144 Hz compositor cadence target
- double buffering and dirty-rectangle present
- direct PCI enumeration
- i8042 input probe
- fair-queue scheduler foundation
- native Zorix syscall dispatcher
- ELF64 executable validation
- VFS metadata foundation
- pipe ring-buffer IPC
- futex wait/wake state foundation
- QEMU/OVMF framebuffer validation

## Smoothness target

The 0.3 timing model uses:

- 1000 Hz kernel/input/scheduler timing
- 144 Hz desktop compositor target
- dirty rectangles instead of mandatory full-screen redraw
- double buffering
- no busy-wait compositor loop
- bounded refresh work

A higher numerical frame target is not automatically smoother. The next
graphics work should add adaptive 60/90/120/144/165 Hz output selection based
on real display capabilities and measured render cost.

## Still required before current desktop apps can run

- real preemptive context switching
- ring-3 execution
- independent page tables per process
- physical page allocator and VM mappings
- complete ELF program-header mapping
- syscall entry from user mode
- signals, threads, TLS and blocking futex queues
- VFS mounts and real filesystems
- sockets, epoll/poll, PTYs
- AHCI/NVMe drivers
- USB xHCI/HID
- Ethernet/Wi-Fi
- audio
- Bluetooth
- native service manager
- graphics/input/audio userspace APIs
- selected Linux ABI compatibility

The public Zorix desktop must remain on its validated Linux kernel until these
items are actually implemented and the Glass desktop boots on Zorix Kernel.

## CI success markers

The native boot CI requires timer, paging, scheduler, syscall, ELF, VFS, pipe,
futex and 144 Hz compositor selftests plus a nonblank framebuffer capture.
