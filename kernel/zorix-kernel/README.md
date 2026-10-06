# Zorix Kernel 0.2

Zorix Kernel is the independent native-kernel track for Zorix OS.

Version 0.2 moves beyond the UEFI handoff prototype and introduces the first
desktop-oriented kernel foundation.

## 0.2 highlights

- independent x86-64 kernel, not a Linux fork
- retained EFI memory-map inspection after ExitBootServices
- bootstrap kernel heap
- direct framebuffer backbuffer
- dirty-rectangle partial presentation
- compositor micro-benchmark
- IDT breakpoint self-test
- direct PCI bus discovery
- storage/network/display/audio/USB class counts
- input-controller probe
- QEMU/OVMF validation

The current public Zorix ISO still uses Debian Linux until the compatibility
and driver milestones in DESKTOP_COMPAT.md are complete.

## Performance rules

- no unbounded retries
- no full-screen repaint when a dirty rectangle is enough
- no mandatory blur
- bounded frame queues
- event-driven input and rendering
- hardware probes stay off the UI critical path
- low-resource mode is a first-class target

## CI success markers

The native boot CI requires:

- ZORIX_KERNEL_STAGE:boot-services-exited
- ZORIX_KERNEL_MEMORY:heap-ok
- ZORIX_KERNEL_IDT:breakpoint-ok
- ZORIX_KERNEL_STAGE:pci-ready
- ZORIX_KERNEL_RENDER:dirty-rect-present-ok
- ZORIX_KERNEL_STAGE:compositor-ready
- ZORIX_KERNEL_STATUS:desktop-foundation-running
