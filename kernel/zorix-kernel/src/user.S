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

.macro SYMBOL id,x,y,size,color,alpha
    movq $\id, %rbx
    movabsq $((\x << 32) | \y), %r10
    movabsq $((\size << 32) | (\alpha << 24) | \color), %r11
    movq $30, %rax
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
    cmpl $0x00070000, %eax
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

    /* Soft tide-like wallpaper lights. */
    ROUND 40,96,430,486,0x1677d8,78,180
    ROUND 646,34,1018,406,0x865be6,66,180
    ROUND 490,432,930,790,0x25c6a8,48,180

    /* System menu bar. */
    GLASS 8,8,1016,48,0x11283f,102,16,10,78
    TEXT .Lbrand,28,21,0xf5f9fc,1
    TEXT .Lmenus,118,21,0xdfeaf2,1
    SYMBOL 4,862,14,26,0xeaf5fb,228
    SYMBOL 5,900,14,26,0xeaf5fb,228
    SYMBOL 6,938,14,30,0xeaf5fb,228
    TEXT .Lclock,976,21,0xf3f8fb,1

    /* A quiet desktop workspace preview behind apps. */
    GLASS 138,92,886,628,0x122a42,104,28,12,86
    ROUND 138,92,886,150,0xffffff,10,28
    ROUND 160,114,174,128,0xff5f57,238,7
    ROUND 182,114,196,128,0xfebb2e,238,7
    ROUND 204,114,218,128,0x28c840,238,7
    TEXT .Lworkspace,246,111,0xf2f8fc,2

    /* Sidebar uses icons + labels rather than decorative blocks. */
    ROUND 156,170,304,596,0x071521,62,18
    ROUND 168,190,292,230,0xffffff,18,12
    SYMBOL 10,176,196,26,0x72c9ff,235
    TEXT .Lhome,210,202,0xffffff,1
    SYMBOL 0,176,248,26,0xc8dce9,210
    TEXT .Lfiles,210,254,0xdbe8f1,1
    SYMBOL 1,176,300,26,0xc8dce9,210
    TEXT .Lsettings,210,306,0xdbe8f1,1
    SYMBOL 2,176,352,26,0xc8dce9,210
    TEXT .Lterminal,210,358,0xdbe8f1,1

    /* Content hierarchy, kept light so native apps can sit above it. */
    TEXT .Lwelcome,334,184,0xffffff,2
    TEXT .Lsubtitle,334,218,0xa9c2d4,1
    GLASS 334,260,850,358,0x18405f,72,20,9,54
    SYMBOL 11,356,282,44,0x80d0ff,230
    TEXT .Ldisplaycard,416,286,0xf4f9fc,1
    TEXT .Ldisplaydetail,416,312,0xadd0e7,1
    GLASS 334,378,586,512,0x173c54,66,20,9,48
    SYMBOL 4,354,401,40,0x7fe8b1,230
    TEXT .Lnetworkcard,408,405,0xf4f9fc,1
    TEXT .Lonlinetxt,408,432,0x96dab9,1
    GLASS 606,378,850,512,0x253456,62,20,9,48
    SYMBOL 5,626,401,40,0xb6a0ff,230
    TEXT .Laudiocard,680,405,0xf4f9fc,1
    TEXT .Laudiotxt,680,432,0xcabdf4,1

    /* Floating app dock with semantic symbols. */
    GLASS 284,686,740,756,0x10263c,112,24,12,92

    ROUND 304,699,352,747,0x4eaef8,190,14
    SYMBOL 0,312,707,32,0xffffff,245

    ROUND 366,699,414,747,0x7a7ff4,170,14
    SYMBOL 1,374,707,32,0xffffff,245

    ROUND 428,699,476,747,0x27374a,210,14
    SYMBOL 2,436,707,32,0xeef7ff,245

    ROUND 490,699,538,747,0xe96d91,170,14
    SYMBOL 8,498,707,32,0xffffff,245

    ROUND 552,699,600,747,0x4fbb8b,170,14
    SYMBOL 4,560,707,32,0xffffff,245

    ROUND 614,699,662,747,0x6073e5,170,14
    SYMBOL 11,622,707,32,0xffffff,245

    ROUND 676,699,724,747,0xe9edf1,190,14
    SYMBOL 7,684,707,32,0x26394b,245

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

    /* Restore a clean desktop before launching real apps. This avoids
     * duplicated text/windows showing through translucent application glass. */
    xorq %rbx, %rbx
    movq %r12, %r10
    shlq $32, %r10
    orq %r13, %r10
    movabsq $((0x102a4d << 24) | 0x07111f), %r11
    movq $19, %rax
    int $0x80

    ROUND 40,96,430,486,0x1677d8,78,180
    ROUND 646,34,1000,406,0x865be6,66,180
    ROUND 490,432,930,760,0x25c6a8,48,180

    GLASS 8,8,1016,48,0x11283f,102,16,10,78
    TEXT .Lbrand,28,21,0xf5f9fc,1
    TEXT .Lmenus,118,21,0xdfeaf2,1
    SYMBOL 4,862,14,26,0xeaf5fb,228
    SYMBOL 5,900,14,26,0xeaf5fb,228
    SYMBOL 6,938,14,30,0xeaf5fb,228
    TEXT .Lclock,976,21,0xf3f8fb,1

    GLASS 284,686,740,756,0x10263c,112,24,12,92
    ROUND 304,699,352,747,0x4eaef8,190,14
    SYMBOL 0,312,707,32,0xffffff,245
    ROUND 366,699,414,747,0x7a7ff4,170,14
    SYMBOL 1,374,707,32,0xffffff,245
    ROUND 428,699,476,747,0x27374a,210,14
    SYMBOL 2,436,707,32,0xeef7ff,245
    ROUND 490,699,538,747,0xe96d91,170,14
    SYMBOL 8,498,707,32,0xffffff,245
    ROUND 552,699,600,747,0x4fbb8b,170,14
    SYMBOL 4,560,707,32,0xffffff,245
    ROUND 614,699,662,747,0x6073e5,170,14
    SYMBOL 11,622,707,32,0xffffff,245
    ROUND 676,699,724,747,0xe9edf1,190,14
    SYMBOL 7,684,707,32,0x26394b,245

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

.Lbrand:         .asciz "ZORIX"
.Lmenus:         .asciz "FILES   EDIT   VIEW   GO   WINDOW   HELP"
.Lclock:         .asciz "08:33"
.Lworkspace:     .asciz "ZORIX DESKTOP"
.Lhome:          .asciz "HOME"
.Lfiles:         .asciz "FILES"
.Lsettings:      .asciz "SETTINGS"
.Lterminal:      .asciz "TERMINAL"
.Lwelcome:       .asciz "WELCOME BACK"
.Lsubtitle:      .asciz "YOUR NATIVE WORKSPACE IS READY"
.Ldisplaycard:   .asciz "DISPLAY"
.Ldisplaydetail: .asciz "144 HZ LIQUID GLASS"
.Lnetworkcard:   .asciz "NETWORK"
.Lonlinetxt:     .asciz "CONNECTED"
.Laudiocard:     .asciz "SOUND"
.Laudiotxt:      .asciz "48 KHZ OUTPUT"

.Ltitle:    .asciz "ZORIX GLASS"
.Loverview: .asciz "OVERVIEW"
.Lsystem:   .asciz "SYSTEM"
.Lnetwork:  .asciz "NETWORK"
.Ldisplay:  .asciz "DISPLAY"
.Laudio:    .asciz "AUDIO"
.Lstatus:   .asciz "ONLINE"

zk_user_blob_end:
