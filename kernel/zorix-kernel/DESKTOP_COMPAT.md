# Zorix Kernel 0.5 — Desktop Migration Matrix

This file separates **verified execution** from **foundation code**. A feature
must not be described as fully supported unless its end-to-end path is tested.

## Verified end-to-end in QEMU

| Area | Status | Evidence scope |
| --- | --- | --- |
| Native kernel boot | Verified | UEFI -> ExitBootServices -> kernel_main |
| Ring-3 userspace | Verified | CPL3 entry, syscall, return to kernel |
| Glass core shell | Verified | rendered from CPL3 through native graphics syscalls |
| Pointer input | Verified | QMP -> PS/2 -> kernel -> userspace |
| Window focus/drag | Verified | injected drag produces focus and drag markers |
| Keyboard input | Verified | PS/2 queue + native userspace event syscall |
| Native apps | Verified | Settings and Files enter CPL3 with distinct PIDs |
| ELF loader core | Verified | PT_LOAD copy + BSS zero + relocated entry |
| RAMFS | Verified | create/write/read |
| ZorixFS core | Verified | format/mount/create/write/read |
| PCI binding | Verified in QEMU | AHCI, NVMe, xHCI, virtio-net, HDA |
| 144 Hz scheduling | Verified | 143-144 scheduled frames per 1 s window |

## Implemented foundations, not complete hardware support

| Area | Implemented now | Still required |
| --- | --- | --- |
| AHCI | PCI bind + READ DMA EXT command builder | HBA reset, command lists, PRDT DMA, interrupts, error recovery |
| NVMe | PCI bind + read SQE builder | controller enable, admin queue, namespace discovery, DMA/CQ |
| xHCI | PCI bind + transfer TRB builder | controller reset, rings, slots, endpoints, USB enumeration |
| Ethernet | PCI bind + Ethernet/ARP/IPv4/UDP core | NIC queues, RX/TX DMA, DHCP, DNS, TCP, sockets |
| Wi-Fi | 802.11 frame parser/build foundation | chipset drivers, firmware, scan/auth/association, WPA |
| Bluetooth | HCI command/event state core | USB/UART transport, discovery, pairing, profiles |
| Audio | HDA verb builder + 48 kHz mixer/ring | codec topology, BDL DMA, playback/capture IRQ |
| Linux compatibility | syscall subset | full ELF interp, signals, clone, epoll, sockets, ioctl, proc/sys, glibc ABI |
| Installer | GPT/ESP/root planner + CRC validation | storage writes, format, file copy, EFI entry, reboot validation |
| Glass migration | shell render/input/focus/drag | remaining legacy apps, text/font stack, settings backends, accessibility |

## Release rule

Do not replace the Linux kernel in the public installer ISO until all of these
are verified on a blank virtual disk and then on real hardware:

1. Storage read/write through native AHCI/NVMe.
2. ZorixFS/root mount.
3. USB keyboard/mouse through xHCI.
4. Network through at least one native NIC driver.
5. Native installer writes GPT, ESP and root filesystem.
6. UEFI boot from the installed disk.
7. Glass starts and accepts input after reboot.
8. Hardware matrix includes at least one Intel, AMD and common laptop target.

Until then, Zorix Kernel is an independent native kernel under active migration,
while the public desktop ISO remains Linux-backed.
