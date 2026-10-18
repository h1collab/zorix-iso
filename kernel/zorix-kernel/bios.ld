/* Zorix Native BIOS image layout: loaded by Multiboot2 GRUB. */
OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)
ENTRY(zk_multiboot_entry)
SECTIONS
{
  . = 0x01000000;
  .multiboot ALIGN(8) : { KEEP(*(.multiboot)) }
  .text ALIGN(4096) : { *(.text.boot) *(.text .text.*) }
  .rodata ALIGN(4096) : { *(.rodata.boot) *(.rodata .rodata.*) }
  .data ALIGN(4096) : { *(.data .data.*) }
  .bss ALIGN(4096) : { *(COMMON) *(.bss.boot) *(.bss .bss.*) }
}
