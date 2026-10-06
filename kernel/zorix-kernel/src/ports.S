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

.globl zk_sti
zk_sti:
    sti
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

.globl zk_read_cr3
zk_read_cr3:
    movq %cr3, %rax
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


.extern zk_timer_ticks
.globl zk_irq0
zk_irq0:
    pushq %rax
    pushq %rdx
    incq zk_timer_ticks(%rip)
    movb $0x20, %al
    movw $0x20, %dx
    outb %al, %dx
    popq %rdx
    popq %rax
    iretq


.globl zk_lgdt
zk_lgdt:
    lgdt (%rcx)
    ret

.globl zk_ltr
zk_ltr:
    movw %cx, %ax
    ltr %ax
    ret

.globl zk_enter_user
zk_enter_user:
    /* Windows x64 ABI: rcx=entry, rdx=user_rsp */
    pushq $0x23
    pushq %rdx
    pushfq
    orq $0x200, (%rsp)
    pushq $0x1b
    pushq %rcx
    iretq

.extern zk_syscall_int80_dispatch
.globl zk_int80
zk_int80:
    pushq %rcx
    pushq %rdx
    pushq %r8
    pushq %r9
    subq $32, %rsp
    movq %rax, %rcx
    call zk_syscall_int80_dispatch
    addq $32, %rsp
    popq %r9
    popq %r8
    popq %rdx
    popq %rcx
    iretq
