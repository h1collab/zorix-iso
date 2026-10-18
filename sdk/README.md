# Zorix Native SDK 0.6

The Zorix SDK targets the independent Zorix Kernel userspace ABI. It does not
require Linux, glibc, systemd, X11, Wayland or Chromium.

## Current ABI

- x86-64 ring-3 applications
- `int 0x80` syscall transport
- `rax`: syscall number
- `rbx`: argument 0
- `r10`: argument 1
- `r11`: argument 2
- `rax`: return value
- ABI version: `0x00060000`

## Native UI

`include/zorix/zorix.h` exposes framebuffer-independent native UI calls for:

- screen information
- rectangle/present
- Liquid Glass panels
- rounded alpha layers
- text
- system symbols
- timer sleep
- pointer and keyboard snapshots
- process identity

The compositor remains owned by Zorix Kernel. Applications submit native UI
operations from ring 3 and do not access GOP framebuffer memory directly.

## Build

The SDK sample is freestanding and can be syntax-checked with Clang:

```sh
clang --target=x86_64-pc-windows-msvc -ffreestanding -mno-red-zone \
  -fno-stack-protector -fno-builtin -I sdk/include \
  -c sdk/examples/hello.c -o hello.obj
```

The next SDK milestone is a standalone Zorix ELF app linker/runtime so third
party applications can be installed by Zorix Store rather than compiled into
the kernel image.
