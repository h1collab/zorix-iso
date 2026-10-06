.text
.globl zk_user_blob_start
.globl zk_user_blob_end

/* Zorix native userspace syscall ABI:
 * rax = nr, rbx = arg0, r10 = arg1, r11 = arg2
 */
.macro DRAW x0,y0,x1,y1,color
    movabsq $((\x0 << 32) | \y0), %rbx
    movabsq $((\x1 << 32) | \y1), %r10
    movq $\color, %r11
    movq $10, %rax
    int $0x80
.endm

zk_user_blob_start:
    movw %cs, %ax
    andl $3, %eax
    cmpl $3, %eax
    jne .Lfail

    /* Basic native ABI probes. */
    movq $0, %rax
    int $0x80
    cmpq $42, %rax
    jne .Lfail

    movq $1, %rax
    int $0x80
    cmpl $0x00050000, %eax
    jne .Lfail

    movq $2, %rax
    int $0x80
    testq %rax, %rax
    jz .Lfail

    movq $6, %rax
    int $0x80
    cmpq $1, %rax
    jne .Lfail

    movq $7, %rax
    int $0x80
    cmpq $1, %rax
    jne .Lfail

    movq $8, %rax
    int $0x80
    cmpq $1, %rax
    jne .Lfail

    /* Query screen dimensions. */
    movq $9, %rax
    int $0x80
    movq %rax, %r12
    shrq $32, %r12              /* width */
    movl %eax, %r13d            /* height */
    cmpl $640, %r12d
    jb .Lfail
    cmpl $480, %r13d
    jb .Lfail

    /* Full Glass background. */
    xorq %rbx, %rbx
    movq %r12, %r10
    shlq $32, %r10
    orq %r13, %r10
    movq $0x07111f, %r11
    movq $10, %rax
    int $0x80

    /* Top menu bar and left launcher rail. */
    movabsq $0, %rbx
    movabsq $0x000004000000002a, %r10
    movq $0x101d31, %r11
    movq $10, %rax
    int $0x80

    movabsq $0x000000000000002a, %rbx
    movabsq $0x0000003a00000300, %r10
    movq $0x0c1828, %r11
    movq $10, %rax
    int $0x80

    /* Zorix mark in top-left. */
    DRAW 14,10,30,26,0x2296f3
    DRAW 20,14,35,18,0xe9f5ff
    DRAW 18,22,33,26,0xe9f5ff

    /* Status cluster: network / audio / battery. */
    DRAW 878,12,894,28,0x35d07f
    DRAW 902,12,918,28,0x4aa9ff
    DRAW 926,12,956,28,0x8ce99a
    DRAW 958,16,963,24,0xe9f5ff

    /* Main native window. */
    DRAW 154,104,892,652,0x15243a
    DRAW 154,104,892,146,0x203652
    DRAW 177,118,189,130,0xff6b6b
    DRAW 199,118,211,130,0xffca61
    DRAW 221,118,233,130,0x59d987

    /* Window sidebar. */
    DRAW 176,168,326,624,0x101c2d
    DRAW 196,190,302,226,0x1e3552
    DRAW 196,244,302,280,0x16283e
    DRAW 196,298,302,334,0x16283e
    DRAW 196,352,302,388,0x16283e

    /* Main content cards. */
    DRAW 350,172,858,276,0x1b304a
    DRAW 372,194,544,214,0x4aa9ff
    DRAW 372,230,816,244,0x8aa4bf

    DRAW 350,296,592,440,0x172a41
    DRAW 616,296,858,440,0x172a41
    DRAW 374,320,568,338,0x35d07f
    DRAW 640,320,834,338,0xaa7cff
    DRAW 374,362,550,412,0x203b59
    DRAW 640,362,816,412,0x203b59

    DRAW 350,464,858,606,0x13243a
    DRAW 374,488,560,506,0xffcc66
    DRAW 374,526,824,542,0x294461
    DRAW 374,558,748,574,0x20374f

    /* Left launcher icons. */
    DRAW 10,72,48,110,0x2296f3
    DRAW 10,124,48,162,0x35d07f
    DRAW 10,176,48,214,0xffb44a
    DRAW 10,228,48,266,0xaa7cff

    /* Dynamic bottom dock using height. */
    movl %r13d, %r14d
    subl $78, %r14d
    movl %r13d, %r15d
    subl $18, %r15d

    /* Dock background x=274..750 */
    movq $274, %rbx
    shlq $32, %rbx
    orq %r14, %rbx
    movq $750, %r10
    shlq $32, %r10
    orq %r15, %r10
    movq $0x16263b, %r11
    movq $10, %rax
    int $0x80

    /* Seven dock app icons. */
    movl %r14d, %eax
    addl $11, %eax
    movl %eax, %r8d
    movl %r14d, %eax
    addl $49, %eax
    movl %eax, %r9d

    movq $292, %rbx
    shlq $32, %rbx
    orq %r8, %rbx
    movq $330, %r10
    shlq $32, %r10
    orq %r9, %r10
    movq $0x2296f3, %r11
    movq $10, %rax
    int $0x80

    movq $354, %rbx
    shlq $32, %rbx
    orq %r8, %rbx
    movq $392, %r10
    shlq $32, %r10
    orq %r9, %r10
    movq $0x35d07f, %r11
    movq $10, %rax
    int $0x80

    movq $416, %rbx
    shlq $32, %rbx
    orq %r8, %rbx
    movq $454, %r10
    shlq $32, %r10
    orq %r9, %r10
    movq $0xffb44a, %r11
    movq $10, %rax
    int $0x80

    movq $478, %rbx
    shlq $32, %rbx
    orq %r8, %rbx
    movq $516, %r10
    shlq $32, %r10
    orq %r9, %r10
    movq $0xaa7cff, %r11
    movq $10, %rax
    int $0x80

    movq $540, %rbx
    shlq $32, %rbx
    orq %r8, %rbx
    movq $578, %r10
    shlq $32, %r10
    orq %r9, %r10
    movq $0xff6b8a, %r11
    movq $10, %rax
    int $0x80

    movq $602, %rbx
    shlq $32, %rbx
    orq %r8, %rbx
    movq $640, %r10
    shlq $32, %r10
    orq %r9, %r10
    movq $0x4ad7d1, %r11
    movq $10, %rax
    int $0x80

    movq $664, %rbx
    shlq $32, %rbx
    orq %r8, %rbx
    movq $702, %r10
    shlq $32, %r10
    orq %r9, %r10
    movq $0xe9f5ff, %r11
    movq $10, %rax
    int $0x80

    /* Present full screen from userspace. */
    xorq %rbx, %rbx
    movq %r12, %r10
    shlq $32, %r10
    orq %r13, %r10
    xorq %r11, %r11
    movq $11, %rax
    int $0x80

    /* Explicit native desktop readiness marker. */
    movq $12, %rax
    int $0x80

    /* Real pointer/focus/drag event loop. Runs for about 1.6 seconds
     * during CI and can be driven by QEMU's PS/2 mouse injection. */
    xorl %r14d, %r14d          /* 0 idle, 1 focused, 2 dragged */
    movl $200, %r15d
.Linput_loop:
    movq $14, %rax
    int $0x80
    movq %rax, %r8
    movq %rax, %r9
    shrq $32, %r8             /* x */
    shrq $8, %r9
    andq $0x00ffffff, %r9     /* y */
    movl %eax, %ecx
    andl $7, %ecx             /* buttons */

    /* Any decoded packet proves the native pointer path is live. */
    testl %ecx, %ecx
    jz .Linput_sleep

    /* Left button in the title bar focuses the window. */
    testl $1, %ecx
    jz .Linput_sleep
    cmpl $154, %r8d
    jb .Linput_sleep
    cmpl $892, %r8d
    ja .Linput_sleep
    cmpl $104, %r9d
    jb .Linput_sleep
    cmpl $180, %r9d
    ja .Linput_sleep

    cmpl $0, %r14d
    jne .Ldrag_window
    movq $1, %rbx
    movq $15, %rax
    int $0x80
    movl $1, %r14d

.Ldrag_window:
    /* Move a complete focused Glass window to the pointer position.
     * Clamp so it remains on-screen. */
    movl %r8d, %edx
    subl $180, %edx
    cmpl $64, %edx
    jae .Ldrag_x_low_ok
    movl $64, %edx
.Ldrag_x_low_ok:
    cmpl $460, %edx
    jbe .Ldrag_x_ok
    movl $460, %edx
.Ldrag_x_ok:

    movl %r9d, %esi
    subl $20, %esi
    cmpl $60, %esi
    jae .Ldrag_y_low_ok
    movl $60, %esi
.Ldrag_y_low_ok:
    cmpl $390, %esi
    jbe .Ldrag_y_ok
    movl $390, %esi
.Ldrag_y_ok:

    /* Clear previous central region before moving the window. */
    movabsq $0x0000005000000040, %rbx
    movabsq $0x000003c0000002b0, %r10
    movq $0x07111f, %r11
    movq $10, %rax
    int $0x80

    /* Body */
    movq %rdx, %rbx
    shlq $32, %rbx
    orq %rsi, %rbx
    movl %edx, %eax
    addl $500, %eax
    movq %rax, %r10
    shlq $32, %r10
    movl %esi, %eax
    addl $320, %eax
    orq %rax, %r10
    movq $0x15243a, %r11
    movq $10, %rax
    int $0x80

    /* Focused title bar */
    movq %rdx, %rbx
    shlq $32, %rbx
    orq %rsi, %rbx
    movl %edx, %eax
    addl $500, %eax
    movq %rax, %r10
    shlq $32, %r10
    movl %esi, %eax
    addl $42, %eax
    orq %rax, %r10
    movq $0x2c537d, %r11
    movq $10, %rax
    int $0x80

    /* Re-present the central desktop region. */
    movabsq $0x0000005000000040, %rbx
    movabsq $0x000003c0000002b0, %r10
    xorq %r11, %r11
    movq $11, %rax
    int $0x80

    cmpl $2, %r14d
    je .Linput_sleep
    movq $2, %rbx
    movq $15, %rax
    int $0x80
    movl $2, %r14d

.Linput_sleep:
    movq $8, %rbx
    movq $13, %rax
    int $0x80
    decl %r15d
    jnz .Linput_loop

    /* Ring3 proof marker. */
    movq $4, %rax
    int $0x80

    /* Exit back to kernel continuation. */
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
