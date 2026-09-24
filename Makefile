CC ?= gcc
SWIFTC ?= swiftc
CLANG ?= clang
LLDLINK ?= lld-link
CFLAGS = -O2 -Wall -Wextra
WORK ?= /mnt/data/zorix-build-0.7
KERNEL ?= /boot/vmlinuz-6.12.96+deb13-amd64
OUTPUT ?= zorix-os-0.7-liquid-glass-amd64.iso

all: bin/BOOTX64.EFI bin/init bin/zorix-core bin/zorix-framebuffer bin/zorix-input bin/zorix-live-power rust-optional
bin:
	mkdir -p bin
bin/BOOTX64.EFI: src/efi_loader.c | bin
	$(CLANG) --target=x86_64-pc-windows-msvc -ffreestanding -fshort-wchar -mno-red-zone -fno-stack-protector -fno-builtin $(CFLAGS) -c $< -o bin/efi_loader.obj
	$(LLDLINK) /subsystem:efi_application /entry:efi_main /nodefaultlib /machine:x64 /out:$@ bin/efi_loader.obj
bin/init: src/live_init.c | bin
	$(CC) $(CFLAGS) -static $< -o $@
bin/zorix-core: src/ZorixCore.swift | bin
	$(SWIFTC) -O $< -o $@ -Xlinker -rpath -Xlinker /usr/lib/zorix/swift
rust-optional:
	./tools/build_rust_optional.sh
.PHONY: all rust-optional
