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

.macro GRAD x0,y0,x1,y1,top,bottom
    movabsq $((\x0 << 32) | \y0), %rbx
    movabsq $((\x1 << 32) | \y1), %r10
    movabsq $((\bottom << 24) | \top), %r11
    movq $19, %rax
    int $0x80
.endm

.macro GLASS x0,y0,x1,y1,tint,alpha,radius,blur,border
    movabsq $((\x0 << 32) | \y0), %rbx
    movabsq $((\x1 << 32) | \y1), %r10
    movabsq $((\border << 48) | (\blur << 40) | (\radius << 32) | (\alpha << 24) | \tint), %r11
    movq $20, %rax
    int $0x80
.endm

.macro ROUND x0,y0,x1,y1,color,alpha,radius
    movabsq $((\x0 << 32) | \y0), %rbx
    movabsq $((\x1 << 32) | \y1), %r10
    movabsq $((\radius << 32) | (\alpha << 24) | \color), %r11
    movq $21, %rax
    int $0x80
.endm

.macro TEXT label,x,y,color,scale
    leaq \label(%rip), %rbx
    movabsq $((\x << 32) | \y), %r10
    movabsq $((\scale << 24) | \color), %r11
    movq $22, %rax
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

    /* Pixel-level Liquid Glass desktop. */
    /* Full framebuffer gradient: use runtime width/height, not a 1024x768 assumption. */
    xorq %rbx, %rbx
    movq %r12, %r10
    shlq $32, %r10
    orq %r13, %r10
    movabsq $((0x102a4d << 24) | 0x07111f), %r11
    movq $19, %rax
    int $0x80

    /* Soft aurora lights behind the glass. */
    ROUND 58,100,410,452,0x157efb,92,150
    ROUND 650,54,1000,404,0x9b5cff,78,150
    ROUND 520,430,900,760,0x23d7b2,58,150

    /* Frosted menu bar and side rail. */
    GLASS 14,12,1010,58,0x183453,118,18,10,100
    GLASS 14,76,70,368,0x102b46,132,22,10,90

    /* Main floating Glass window. */
    GLASS 126,92,914,650,0x142d49,138,28,12,118
    GLASS 148,116,892,158,0x244d73,106,18,9,82

    /* Traffic-light controls with real alpha/rounding. */
    ROUND 170,128,184,142,0xff6b7d,240,7
    ROUND 194,128,208,142,0xffc85a,240,7
    ROUND 218,128,232,142,0x55dc91,240,7

    /* Sidebar and translucent cards. */
    GLASS 150,178,316,616,0x0e2339,118,22,9,70
    GLASS 338,178,886,290,0x1b4166,112,22,11,78
    GLASS 338,310,598,458,0x173954,104,22,10,70
    GLASS 618,310,886,458,0x2c2a61,94,22,10,70
    GLASS 338,478,886,610,0x112e48,106,22,10,70

    /* Sidebar selections and content accents. */
    ROUND 170,204,294,244,0x5eb8ff,54,14
    ROUND 170,260,294,300,0xffffff,18,14
    ROUND 170,316,294,356,0xffffff,14,14
    ROUND 170,372,294,412,0xffffff,12,14

    ROUND 364,212,572,224,0x71c9ff,228,6
    ROUND 364,244,838,252,0xd8efff,132,4
    ROUND 364,338,570,354,0x4fe2a1,220,8
    ROUND 644,338,854,354,0xb48aff,220,8
    ROUND 366,390,566,428,0x7ab8ed,42,14
    ROUND 646,390,846,428,0xa28cff,38,14
    ROUND 364,506,566,522,0xffd36a,220,8
    ROUND 364,546,842,556,0xb7d8f6,70,5
    ROUND 364,574,760,584,0x9ec7ed,52,5

    /* Glass launcher capsules. */
    GLASS 18,84,66,132,0x1e73b9,132,16,7,80
    GLASS 18,142,66,190,0x159a71,126,16,7,80
    GLASS 18,200,66,248,0xa66d17,122,16,7,80
    GLASS 18,258,66,306,0x7449b6,122,16,7,80

    /* Bottom floating dock. */
    GLASS 252,688,772,758,0x152f4b,126,24,12,112
    ROUND 278,704,322,748,0x249cf0,225,13
    ROUND 340,704,384,748,0x36d58b,225,13
    ROUND 402,704,446,748,0xffb84f,225,13
    ROUND 464,704,508,748,0x9b72ef,225,13
    ROUND 526,704,570,748,0xff6688,225,13
    ROUND 588,704,632,748,0x45ced0,225,13
    ROUND 650,704,694,748,0xeaf6ff,225,13

    /* Native bitmap typography rendered in CPL3 through the Glass ABI. */
    TEXT .Ltitle,166,137,0xf2f8ff,2
    TEXT .Loverview,364,198,0xe9f6ff,2
    TEXT .Lsystem,176,218,0xffffff,1
    TEXT .Lnetwork,176,274,0xd8ebff,1
    TEXT .Ldisplay,176,330,0xd8ebff,1
    TEXT .Laudio,176,386,0xd8ebff,1
    TEXT .Lstatus,796,31,0xdff6ff,1

    /* Prove all liquid-glass primitives were exercised. */
    movq $23, %rax
    int $0x80
    cmpq $1, %rax
    jne .Lfail

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

    /* Focused dragged window uses the same translucent Glass compositor. */
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
    movabsq $0x705e1c8a18355a, %r11
    movq $20, %rax
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
    movq $17, %rax
    int $0x80
    testq %rax, %rax
    jz .Lsleep_tick
    movq $18, %rax
    int $0x80

.Lsleep_tick:
    movq $8, %rbx
    movq $13, %rax
    int $0x80
    decl %r15d
    jnz .Linput_loop

    /* Repaint the final focused desktop after CI drag so the captured frame
     * keeps full content instead of the simplified drag proxy. */
    GLASS 126,92,914,650,0x142d49,138,28,12,118
    GLASS 148,116,892,158,0x244d73,116,18,9,94
    ROUND 170,128,184,142,0xff6b7d,240,7
    ROUND 194,128,208,142,0xffc85a,240,7
    ROUND 218,128,232,142,0x55dc91,240,7
    GLASS 150,178,316,616,0x0e2339,118,22,9,70
    GLASS 338,178,886,290,0x1b4166,112,22,11,78
    GLASS 338,310,598,458,0x173954,104,22,10,70
    GLASS 618,310,886,458,0x2c2a61,94,22,10,70
    GLASS 338,478,886,610,0x112e48,106,22,10,70
    ROUND 170,204,294,244,0x5eb8ff,54,14
    ROUND 364,212,572,224,0x71c9ff,228,6
    ROUND 364,244,838,252,0xd8efff,132,4
    ROUND 364,338,570,354,0x4fe2a1,220,8
    ROUND 644,338,854,354,0xb48aff,220,8
    ROUND 364,506,566,522,0xffd36a,220,8
    ROUND 364,546,842,556,0xb7d8f6,70,5
    TEXT .Ltitle,166,137,0xf2f8ff,2
    TEXT .Loverview,364,198,0xe9f6ff,2
    TEXT .Lsystem,176,218,0xffffff,1
    TEXT .Lnetwork,176,274,0xd8ebff,1
    TEXT .Ldisplay,176,330,0xd8ebff,1
    TEXT .Laudio,176,386,0xd8ebff,1

    /* Present full framebuffer after final repaint. */
    xorq %rbx, %rbx
    movq %r12, %r10
    shlq $32, %r10
    orq %r13, %r10
    xorq %r11, %r11
    movq $11, %rax
    int $0x80

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

.Ltitle:    .asciz "ZORIX GLASS"
.Loverview: .asciz "OVERVIEW"
.Lsystem:   .asciz "SYSTEM"
.Lnetwork:  .asciz "NETWORK"
.Ldisplay:  .asciz "DISPLAY"
.Laudio:    .asciz "AUDIO"
.Lstatus:   .asciz "ONLINE"

zk_user_blob_end:
