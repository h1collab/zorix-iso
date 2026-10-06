.text

.globl zk_out8
zk_out8:
    movb %dl, %al
    movw %cx, %dx
    outb %al, %dx
    ret

.globl zk_in8
zk_in8:
    movw %cx, %dx
    inb %dx, %al
    movzbl %al, %eax
    ret

.globl zk_out32
zk_out32:
    movl %edx, %eax
    movw %cx, %dx
    outl %eax, %dx
    ret

.globl zk_in32
zk_in32:
    movw %cx, %dx
    inl %dx, %eax
    ret

.globl zk_cli
zk_cli:
    cli
    ret

.globl zk_hlt
zk_hlt:
    hlt
    ret

.globl zk_pause
zk_pause:
    pause
    ret

.globl zk_rdtsc
zk_rdtsc:
    rdtsc
    shlq $32, %rdx
    orq %rdx, %rax
    ret

.globl zk_read_cs
zk_read_cs:
    movw %cs, %ax
    movzwl %ax, %eax
    ret

.globl zk_store_idt
zk_store_idt:
    sidt (%rcx)
    ret

.globl zk_load_idt
zk_load_idt:
    lidt (%rcx)
    ret

.globl zk_test_breakpoint
zk_test_breakpoint:
    int3
    ret

.extern zk_breakpoint_hits
.globl zk_isr3
zk_isr3:
    incq zk_breakpoint_hits(%rip)
    iretq
