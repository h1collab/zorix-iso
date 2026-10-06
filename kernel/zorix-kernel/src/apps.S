.text
.globl zk_settings_blob_start
.globl zk_settings_blob_end
.globl zk_files_blob_start
.globl zk_files_blob_end

/* Shared native UI syscall helpers for real CPL3 apps. */
.macro APP_GLASS x0,y0,x1,y1,tint,alpha,radius,blur,border
    movabsq $((\x0 << 32) | \y0), %rbx
    movabsq $((\x1 << 32) | \y1), %r10
    movabsq $((\border << 48) | (\blur << 40) | (\radius << 32) | (\alpha << 24) | \tint), %r11
    movq $20, %rax
    int $0x80
.endm

.macro APP_ROUND x0,y0,x1,y1,color,alpha,radius
    movabsq $((\x0 << 32) | \y0), %rbx
    movabsq $((\x1 << 32) | \y1), %r10
    movabsq $((\radius << 32) | (\alpha << 24) | \color), %r11
    movq $21, %rax
    int $0x80
.endm

.macro APP_TEXT label,x,y,color,scale
    leaq \label(%rip), %rbx
    movabsq $((\x << 32) | \y), %r10
    movabsq $((\scale << 24) | \color), %r11
    movq $22, %rax
    int $0x80
.endm

.macro APP_PRESENT
    xorq %rbx, %rbx
    movabsq $0x0000040000000300, %r10
    xorq %r11, %r11
    movq $11, %rax
    int $0x80
.endm

/* ---------------- Settings ---------------- */
zk_settings_blob_start:
    movq $0, %rax
    int $0x80
    cmpq $100, %rax
    jne .Lsettings_fail

    /* Query live system snapshot: high32=refresh, low flags=device state. */
    movq $24, %rax
    int $0x80
    movq %rax, %r12
    movl %eax, %r13d

    APP_GLASS 132,86,892,654,0x142d49,144,28,12,120
    APP_GLASS 154,108,870,158,0x244d73,112,18,9,90
    APP_TEXT .Lset_title,180,126,0xf3f9ff,2

    APP_GLASS 154,178,332,626,0x10263e,122,20,9,72
    APP_ROUND 170,202,314,240,0x5db7ff,54,13
    APP_TEXT .Lgeneral,184,216,0xffffff,1
    APP_TEXT .Ldisplay,184,270,0xd5eaff,1
    APP_TEXT .Lnetwork,184,324,0xd5eaff,1
    APP_TEXT .Laudio,184,378,0xd5eaff,1
    APP_TEXT .Linput,184,432,0xd5eaff,1

    APP_GLASS 352,178,870,626,0x173956,112,22,10,76
    APP_TEXT .Ldevice_hdr,382,206,0xffffff,2
    APP_TEXT .Lkernel_label,382,252,0xbfdcf5,1
    APP_TEXT .Lkernel_value,566,252,0xffffff,1
    APP_TEXT .Lrefresh_label,382,286,0xbfdcf5,1
    APP_TEXT .Lrefresh_value,566,286,0x70d3ff,1
    APP_TEXT .Lgraphics_label,382,320,0xbfdcf5,1
    APP_TEXT .Lgraphics_value,566,320,0x7ef0b7,1
    APP_TEXT .Linput_label,382,354,0xbfdcf5,1
    APP_TEXT .Linput_value,566,354,0x7ef0b7,1

    APP_TEXT .Lhardware_hdr,382,410,0xffffff,2

    /* Live PCI flags -> status pills. */
    testl $1, %r13d
    jz .Lnvme_off
    APP_ROUND 382,452,482,480,0x3cd98b,200,12
    APP_TEXT .Lnvme_on,397,461,0xffffff,1
    jmp .Lnvme_done
.Lnvme_off:
    APP_ROUND 382,452,482,480,0xff6b7a,160,12
    APP_TEXT .Lnvme_off_txt,397,461,0xffffff,1
.Lnvme_done:

    testl $2, %r13d
    jz .Lusb_off
    APP_ROUND 494,452,594,480,0x3cd98b,200,12
    APP_TEXT .Lusb_on,511,461,0xffffff,1
    jmp .Lusb_done
.Lusb_off:
    APP_ROUND 494,452,594,480,0xff6b7a,160,12
    APP_TEXT .Lusb_off_txt,511,461,0xffffff,1
.Lusb_done:

    testl $4, %r13d
    jz .Lnet_off
    APP_ROUND 606,452,706,480,0x3cd98b,200,12
    APP_TEXT .Lnet_on,619,461,0xffffff,1
    jmp .Lnet_done
.Lnet_off:
    APP_ROUND 606,452,706,480,0xff6b7a,160,12
    APP_TEXT .Lnet_off_txt,619,461,0xffffff,1
.Lnet_done:

    testl $8, %r13d
    jz .Laud_off
    APP_ROUND 718,452,818,480,0x3cd98b,200,12
    APP_TEXT .Laud_on,730,461,0xffffff,1
    jmp .Laud_done
.Laud_off:
    APP_ROUND 718,452,818,480,0xff6b7a,160,12
    APP_TEXT .Laud_off_txt,730,461,0xffffff,1
.Laud_done:

    APP_GLASS 382,514,838,588,0x204563,86,16,8,60
    APP_TEXT .Lperf_title,402,532,0xffffff,1
    APP_TEXT .Lperf_value,402,558,0xa9dfff,1

    APP_PRESENT

    movq $1, %rbx
    movq $16, %rax
    int $0x80
    movq $27, %rax
    int $0x80

    movq $3, %rax
    int $0x80
    ud2

.Lsettings_fail:
    movq $0x101, %rbx
    movq $16, %rax
    int $0x80
    movq $3, %rax
    int $0x80
    ud2

.Lset_title:       .asciz "SETTINGS"
.Lgeneral:         .asciz "GENERAL"
.Ldisplay:         .asciz "DISPLAY"
.Lnetwork:         .asciz "NETWORK"
.Laudio:           .asciz "AUDIO"
.Linput:           .asciz "INPUT"
.Ldevice_hdr:      .asciz "SYSTEM"
.Lkernel_label:    .asciz "KERNEL"
.Lkernel_value:    .asciz "ZORIX 0.5"
.Lrefresh_label:   .asciz "DISPLAY RATE"
.Lrefresh_value:   .asciz "144 HZ"
.Lgraphics_label:  .asciz "GLASS"
.Lgraphics_value:  .asciz "NATIVE"
.Linput_label:     .asciz "POINTER"
.Linput_value:     .asciz "ACTIVE"
.Lhardware_hdr:    .asciz "HARDWARE"
.Lnvme_on:         .asciz "NVME ON"
.Lnvme_off_txt:    .asciz "NVME OFF"
.Lusb_on:          .asciz "USB ON"
.Lusb_off_txt:     .asciz "USB OFF"
.Lnet_on:          .asciz "NET ON"
.Lnet_off_txt:     .asciz "NET OFF"
.Laud_on:          .asciz "AUDIO ON"
.Laud_off_txt:     .asciz "AUDIO OFF"
.Lperf_title:      .asciz "COMPOSITOR"
.Lperf_value:      .asciz "144 HZ DIRTY RECT"

zk_settings_blob_end:

/* ---------------- Files ---------------- */
zk_files_blob_start:
    movq $0, %rax
    int $0x80
    cmpq $101, %rax
    jne .Lfiles_fail

    movq $25, %rax
    int $0x80
    movl %eax, %r12d
    cmpl $1, %r12d
    jb .Lfiles_fail

    APP_GLASS 108,76,916,666,0x142d49,146,28,12,122
    APP_GLASS 130,98,894,150,0x244d73,112,18,9,92
    APP_TEXT .Lfiles_title,158,116,0xf4faff,2
    APP_TEXT .Lpath,610,122,0xc9e8ff,1

    /* Left locations sidebar. */
    APP_GLASS 130,170,304,638,0x10263e,124,20,9,74
    APP_ROUND 148,194,286,234,0x5cb8ff,54,13
    APP_TEXT .Lhome,166,207,0xffffff,1
    APP_TEXT .Ldesktop,166,262,0xd5eaff,1
    APP_TEXT .Ldocuments,166,312,0xd5eaff,1
    APP_TEXT .Lsystem,166,362,0xd5eaff,1

    /* Main file list. */
    APP_GLASS 326,170,894,476,0x173956,112,22,10,76
    APP_TEXT .Lname_hdr,352,194,0xaed7f7,1
    APP_TEXT .Ltype_hdr,718,194,0xaed7f7,1

    APP_ROUND 344,220,876,258,0x63bfff,36,11
    movq $0, %rbx
    movabsq $0x00000170000000ec, %r10
    movabsq $0x01ffffff, %r11
    movq $26, %rax
    int $0x80
    APP_TEXT .Ltext_type,724,236,0xcce7fb,1

    movq $1, %rbx
    movabsq $0x000001700000011f, %r10
    movabsq $0x01d7eaff, %r11
    movq $26, %rax
    int $0x80
    APP_TEXT .Llog_type,724,287,0xb9d7ef,1

    movq $2, %rbx
    movabsq $0x0000017000000152, %r10
    movabsq $0x01d7eaff, %r11
    movq $26, %rax
    int $0x80
    APP_TEXT .Lcfg_type,724,338,0xb9d7ef,1

    movq $3, %rbx
    movabsq $0x0000017000000185, %r10
    movabsq $0x01d7eaff, %r11
    movq $26, %rax
    int $0x80
    APP_TEXT .Llist_type,724,389,0xb9d7ef,1

    movq $4, %rbx
    movabsq $0x00000170000001b8, %r10
    movabsq $0x01d7eaff, %r11
    movq $26, %rax
    int $0x80
    APP_TEXT .Ltext_type,724,440,0xb9d7ef,1

    /* Real content preview for selected README.TXT. */
    APP_GLASS 326,498,894,638,0x102d48,104,20,9,68
    APP_TEXT .Lpreview_hdr,352,520,0xffffff,1
    APP_TEXT .Lreadme,352,546,0x86ccff,1
    movq $0, %rbx
    movabsq $0x0000017000000230, %r10
    movabsq $0x01eaf6ff, %r11
    movq $29, %rax
    int $0x80

    APP_PRESENT

    movq $2, %rbx
    movq $16, %rax
    int $0x80
    movq $28, %rax
    int $0x80

    movq $3, %rax
    int $0x80
    ud2

.Lfiles_fail:
    movq $0x102, %rbx
    movq $16, %rax
    int $0x80
    movq $3, %rax
    int $0x80
    ud2

.Lfiles_title: .asciz "FILES"
.Lpath:        .asciz "HOME / ZORIX"
.Lhome:        .asciz "HOME"
.Ldesktop:     .asciz "DESKTOP"
.Ldocuments:   .asciz "DOCUMENTS"
.Lsystem:      .asciz "SYSTEM"
.Lname_hdr:    .asciz "NAME"
.Ltype_hdr:    .asciz "TYPE"
.Ltext_type:   .asciz "TEXT"
.Llog_type:    .asciz "LOG"
.Lcfg_type:    .asciz "CONFIG"
.Llist_type:   .asciz "LIST"
.Lpreview_hdr: .asciz "PREVIEW"
.Lreadme:      .asciz "README.TXT"

zk_files_blob_end:
