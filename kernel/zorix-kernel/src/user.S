.text
.globl zk_user_blob_start
.globl zk_user_blob_end

zk_user_blob_start:
    movw %cs, %ax
    andl $3, %eax
    cmpl $3, %eax
    jne .Lfail

    /* getpid() -> 42 */
    movq $0, %rax
    int $0x80
    cmpq $42, %rax
    jne .Lfail

    /* abi_version() -> 0x00040000 */
    movq $1, %rax
    int $0x80
    cmpl $0x00040000, %eax
    jne .Lfail

    /* clock_ticks() must be non-zero */
    movq $2, %rax
    int $0x80
    testq %rax, %rax
    jz .Lfail

    /* pipe round-trip through the syscall surface */
    movq $6, %rax
    int $0x80
    cmpq $1, %rax
    jne .Lfail

    /* VFS root stat through the syscall surface */
    movq $7, %rax
    int $0x80
    cmpq $1, %rax
    jne .Lfail

    /* futex state transition through the syscall surface */
    movq $8, %rax
    int $0x80
    cmpq $1, %rax
    jne .Lfail

    /* Proof marker: this call is only reachable from CPL3. */
    movq $4, %rax
    int $0x80

    /* Exit back to the saved kernel continuation. */
    movq $3, %rax
    int $0x80
    ud2

.Lfail:
    movq $5, %rax
    int $0x80
    movq $3, %rax
    int $0x80
    ud2

zk_user_blob_end:
