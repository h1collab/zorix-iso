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

.globl zk_cli
zk_cli:
    cli
    ret

.globl zk_hlt
zk_hlt:
    hlt
    ret
