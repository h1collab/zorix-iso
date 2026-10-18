/* SPDX-License-Identifier: MIT
 * Transitional BIOS frontend. GRUB Multiboot2 performs the 16-bit/CD
 * bootstrap; Zorix builds its own x86-64 page tables and enters its core.
 * This is not a Linux loader/runtime.
 */
.section .multiboot, "a"
.balign 8
mb2_header_start:
    .long 0xe85250d6
    .long 0
    .long mb2_header_end - mb2_header_start
    .long -(0xe85250d6 + (mb2_header_end - mb2_header_start))
    .short 5                /* optional RGB framebuffer request */
    .short 1
    .long 20
    .long 1024
    .long 768
    .long 32
    .balign 8
    .short 0
    .short 0
    .long 8
mb2_header_end:

.section .text.boot, "ax"
.code32
.globl zk_multiboot_entry
zk_multiboot_entry:
    cli
    movl $bios_stack_top, %esp
    movl %eax, bios_magic
    movl %ebx, bios_info

    /* Identity map all 4 GiB with 2 MiB leaves, without firmware calls. */
    movl $bios_pdpt, %eax
    orl $3, %eax
    movl %eax, bios_pml4
    movl $bios_pdpt, %edi
    movl $bios_pd0, %esi
    movl $4, %ecx
1:
    movl %esi, %eax
    orl $3, %eax
    movl %eax, (%edi)
    addl $8, %edi
    addl $4096, %esi
    loop 1b

    movl $bios_pd0, %edi
    movl $0x83, %eax
    movl $2048, %ecx
2:
    movl %eax, (%edi)
    addl $0x200000, %eax
    addl $8, %edi
    loop 2b

    lgdt bios_gdtr
    movl $bios_pml4, %eax
    movl %eax, %cr3
    movl %cr4, %eax
    orl $0x20, %eax          /* PAE */
    movl %eax, %cr4
    movl $0xc0000080, %ecx
    rdmsr
    orl $0x100, %eax         /* EFER.LME */
    wrmsr
    movl %cr0, %eax
    orl $0x80000000, %eax    /* Paging */
    movl %eax, %cr0
    ljmp $0x08, $zk_bios_long_mode

.code64
zk_bios_long_mode:
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss
    movabsq $bios_stack_top, %rsp
    andq $-16, %rsp
    movl bios_magic(%rip), %edi
    movl bios_info(%rip), %esi
    call zk_bios_entry
3:
    cli
    hlt
    jmp 3b

.section .rodata.boot, "a"
.balign 8
bios_gdt:
    .quad 0
    .quad 0x00af9a000000ffff
    .quad 0x00cf92000000ffff
bios_gdt_end:
bios_gdtr:
    .word bios_gdt_end - bios_gdt - 1
    .long bios_gdt

.section .bss.boot, "aw", @nobits
.balign 4096
bios_pml4: .skip 4096
bios_pdpt: .skip 4096
bios_pd0: .skip 16384
.balign 16
bios_stack: .skip 32768
bios_stack_top:
bios_magic: .skip 4
bios_info: .skip 4
.section .note.GNU-stack,"",@progbits
