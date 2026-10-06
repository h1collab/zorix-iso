# Zorix Kernel 0.2 — Desktop Compatibility Contract

Zorix Kernel 0.2 is an independent kernel foundation. It does not yet claim
binary compatibility with the current Debian/Linux desktop stack.

## Implemented in 0.2

- UEFI x86-64 boot and firmware handoff
- ExitBootServices and retained EFI memory-map inspection
- 16 MiB bootstrap kernel heap
- framebuffer backbuffer
- full-frame initial present
- dirty-rectangle partial present path
- render-cycle benchmark with RDTSC
- IDT load/restore infrastructure and breakpoint self-test
- direct PCI configuration-space enumeration
- desktop-class PCI counters for storage, network, display, audio, USB
- i8042 input-controller probe
- serial diagnostics
- QEMU/OVMF validation

## Desktop blockers still open

The existing Glass desktop uses Chromium, D-Bus, NetworkManager, PipeWire,
BlueZ, Calamares and other Debian/Linux userspace components. Running those
binaries unchanged requires a compatibility surface much larger than this
kernel foundation.

### Process and memory ABI

Required:
- ring-3 process model
- scheduler and preemption
- per-process address spaces
- mmap / mprotect / shared memory
- ELF64 loader
- signals
- threads and TLS
- futexes
- monotonic and realtime clocks

### Filesystem and device ABI

Required:
- VFS
- initramfs and root filesystem
- tmpfs
- proc-like process information
- sys-like hardware information
- device nodes
- permissions and users/groups
- file notification

### I/O ABI

Required:
- pipes
- Unix sockets
- TCP/IP sockets
- poll/select/epoll semantics
- pseudo terminals
- eventfd/timerfd equivalents

### Desktop drivers

Required:
- PCI BAR/resource mapping
- APIC and timer interrupts
- AHCI and NVMe
- USB host controllers and HID
- PS/2 keyboard/touchpad fallback
- GPU modesetting or a native framebuffer strategy
- audio
- Ethernet/Wi-Fi
- Bluetooth

## Performance direction

Zorix should avoid recreating expensive continuous desktop compositing inside
the kernel. The target architecture is:

- event-driven redraws
- retained scene graph in userspace
- double buffering
- dirty-rectangle presentation
- bounded frame queues
- no mandatory full-screen blur on software renderers
- 60 Hz normal target
- 30 Hz low-resource target
- no hardware probe on the compositor/UI critical path

## Migration plan

1. 0.2 — memory, IDT bootstrap, PCI discovery, backbuffer and dirty present.
2. 0.3 — APIC/timer, paging, preemptive scheduler, kernel heap, user mode.
3. 0.4 — VFS, initramfs, ELF loader, syscalls, pipes and sockets.
4. 0.5 — storage/input/USB/network drivers and native service manager.
5. 0.6 — native graphics/input/audio userspace APIs.
6. 0.7 — Glass native shell without Chromium dependency for core desktop.
7. 0.8 — Linux-ABI compatibility subset for selected third-party apps.
8. 1.0 — default Zorix Kernel only after installer and hardware matrix pass.

Until those gates pass, the current public Zorix desktop remains Linux-based.
