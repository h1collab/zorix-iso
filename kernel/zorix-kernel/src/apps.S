.text
.globl zk_settings_blob_start
.globl zk_settings_blob_end
.globl zk_files_blob_start
.globl zk_files_blob_end

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

.macro APP_SYMBOL id,x,y,size,color,alpha
    movq $\id, %rbx
    movabsq $((\x << 32) | \y), %r10
    movabsq $((\size << 32) | (\alpha << 24) | \color), %r11
    movq $30, %rax
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

    movq $24, %rax
    int $0x80
    movl %eax, %r13d

    /* One continuous Tahoe-style material window. */
    APP_GLASS 96,64,928,678,0x12273d,132,26,13,100
    APP_ROUND 96,64,928,128,0xffffff,12,26
    APP_ROUND 112,84,126,98,0xff5f57,238,7
    APP_ROUND 134,84,148,98,0xfebb2e,238,7
    APP_ROUND 156,84,170,98,0x28c840,238,7
    APP_TEXT .Lsettings_title,194,82,0xf7fbff,2

    /* Sidebar is a quiet continuous surface instead of card blocks. */
    APP_ROUND 112,144,300,650,0x071521,72,18
    APP_ROUND 126,174,286,218,0xffffff,20,12
    APP_SYMBOL 1,136,181,28,0x7fc8ff,230
    APP_TEXT .Lgeneral,174,187,0xffffff,1
    APP_SYMBOL 11,136,237,28,0xc7ddf2,210
    APP_TEXT .Ldisplay,174,243,0xd8e7f4,1
    APP_SYMBOL 4,136,293,28,0xc7ddf2,210
    APP_TEXT .Lnetwork,174,299,0xd8e7f4,1
    APP_SYMBOL 5,136,349,28,0xc7ddf2,210
    APP_TEXT .Laudio,174,355,0xd8e7f4,1
    APP_SYMBOL 7,136,405,28,0xc7ddf2,210
    APP_TEXT .Linput,174,411,0xd8e7f4,1

    /* Main detail area: typography and hairline separators. */
    APP_TEXT .Labout,336,166,0xffffff,2
    APP_TEXT .Ldevice_sub,336,198,0xaac2d6,1

    APP_ROUND 336,234,894,235,0xffffff,24,1
    APP_TEXT .Lkernel_label,336,250,0xb8cddd,1
    APP_TEXT .Lkernel_value,664,250,0xffffff,1

    APP_ROUND 336,286,894,287,0xffffff,18,1
    APP_TEXT .Lrefresh_label,336,302,0xb8cddd,1
    APP_TEXT .Lrefresh_value,664,302,0x91d9ff,1

    APP_ROUND 336,338,894,339,0xffffff,18,1
    APP_TEXT .Lgraphics_label,336,354,0xb8cddd,1
    APP_TEXT .Lgraphics_value,664,354,0x8fe7bb,1

    APP_ROUND 336,390,894,391,0xffffff,18,1
    APP_TEXT .Lpointer_label,336,406,0xb8cddd,1
    APP_TEXT .Lpointer_value,664,406,0x8fe7bb,1

    APP_TEXT .Lhardware,336,462,0xffffff,2

    /* Compact status controls, driven by live PCI flags. */
    testl $1, %r13d
    jz .Lnvme_off
    APP_ROUND 336,500,458,534,0x4ade80,52,17
    APP_SYMBOL 9,344,503,26,0x80efae,240
    APP_TEXT .Lnvme_on,376,510,0xeafff2,1
    jmp .Lnvme_done
.Lnvme_off:
    APP_ROUND 336,500,458,534,0xff6b74,42,17
    APP_TEXT .Lnvme_off_txt,352,510,0xffd8dc,1
.Lnvme_done:

    testl $2, %r13d
    jz .Lusb_off
    APP_ROUND 470,500,592,534,0x4ade80,52,17
    APP_SYMBOL 7,478,503,26,0x80efae,240
    APP_TEXT .Lusb_on,512,510,0xeafff2,1
    jmp .Lusb_done
.Lusb_off:
    APP_ROUND 470,500,592,534,0xff6b74,42,17
    APP_TEXT .Lusb_off_txt,486,510,0xffd8dc,1
.Lusb_done:

    testl $4, %r13d
    jz .Lnet_off
    APP_ROUND 604,500,726,534,0x4ade80,52,17
    APP_SYMBOL 4,612,503,26,0x80efae,240
    APP_TEXT .Lnet_on,646,510,0xeafff2,1
    jmp .Lnet_done
.Lnet_off:
    APP_ROUND 604,500,726,534,0xff6b74,42,17
    APP_TEXT .Lnet_off_txt,620,510,0xffd8dc,1
.Lnet_done:

    testl $8, %r13d
    jz .Laud_off
    APP_ROUND 738,500,872,534,0x4ade80,52,17
    APP_SYMBOL 5,746,503,26,0x80efae,240
    APP_TEXT .Laud_on,780,510,0xeafff2,1
    jmp .Laud_done
.Laud_off:
    APP_ROUND 738,500,872,534,0xff6b74,42,17
    APP_TEXT .Laud_off_txt,754,510,0xffd8dc,1
.Laud_done:

    APP_ROUND 336,566,894,630,0xffffff,10,16
    APP_TEXT .Lperf_title,354,581,0xffffff,1
    APP_TEXT .Lperf_value,354,605,0xa8d8f8,1

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

.Lsettings_title:  .asciz "SYSTEM SETTINGS"
.Lgeneral:         .asciz "GENERAL"
.Ldisplay:         .asciz "DISPLAY"
.Lnetwork:         .asciz "NETWORK"
.Laudio:           .asciz "SOUND"
.Linput:           .asciz "INPUT"
.Labout:           .asciz "ABOUT THIS ZORIX"
.Ldevice_sub:      .asciz "NATIVE DESKTOP SYSTEM"
.Lkernel_label:    .asciz "KERNEL"
.Lkernel_value:    .asciz "ZORIX 0.5 NATIVE"
.Lrefresh_label:   .asciz "DISPLAY RATE"
.Lrefresh_value:   .asciz "144 HZ"
.Lgraphics_label:  .asciz "COMPOSITOR"
.Lgraphics_value:  .asciz "LIQUID GLASS"
.Lpointer_label:   .asciz "POINTER AND KEYBOARD"
.Lpointer_value:   .asciz "ACTIVE"
.Lhardware:        .asciz "HARDWARE"
.Lnvme_on:         .asciz "NVME"
.Lnvme_off_txt:    .asciz "NVME OFF"
.Lusb_on:          .asciz "USB"
.Lusb_off_txt:     .asciz "USB OFF"
.Lnet_on:          .asciz "NETWORK"
.Lnet_off_txt:     .asciz "NET OFF"
.Laud_on:          .asciz "AUDIO"
.Laud_off_txt:     .asciz "AUDIO OFF"
.Lperf_title:      .asciz "DISPLAY PERFORMANCE"
.Lperf_value:      .asciz "144 HZ EVENT DRIVEN COMPOSITOR"

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

    /* Full-featured three-column file manager. */
    APP_GLASS 82,54,942,690,0x10243a,134,26,13,104
    APP_ROUND 82,54,942,132,0xffffff,12,26

    /* Window controls and title. */
    APP_ROUND 102,76,116,90,0xff5f57,240,7
    APP_ROUND 124,76,138,90,0xfebb2e,240,7
    APP_ROUND 146,76,160,90,0x28c840,240,7
    APP_SYMBOL 0,190,69,32,0xe7f4ff,235
    APP_TEXT .Lfiles_title,228,76,0xf7fbff,2

    /* Toolbar controls. */
    APP_ROUND 414,70,492,108,0xffffff,14,18
    APP_SYMBOL 7,424,75,28,0xd8eaff,215
    APP_TEXT .Lview,456,83,0xd8eaff,1

    APP_ROUND 666,70,904,108,0xffffff,15,18
    APP_SYMBOL 3,678,75,28,0xc8ddeb,220
    APP_TEXT .Lsearch,714,83,0xa9c0d2,1

    /* Sidebar. */
    APP_ROUND 98,146,284,658,0x06131f,76,18
    APP_TEXT .Lfavorites,116,166,0x8fa8ba,1

    APP_ROUND 108,190,274,232,0xffffff,20,12
    APP_SYMBOL 10,118,197,28,0x70c7ff,235
    APP_TEXT .Lhome,156,204,0xffffff,1

    APP_SYMBOL 0,118,249,28,0xc6d9e8,205
    APP_TEXT .Ldesktop,156,256,0xd9e7f1,1
    APP_SYMBOL 9,118,299,28,0xc6d9e8,205
    APP_TEXT .Ldocuments,156,306,0xd9e7f1,1
    APP_SYMBOL 1,118,349,28,0xc6d9e8,205
    APP_TEXT .Lsystem,156,356,0xd9e7f1,1

    APP_TEXT .Llocations,116,414,0x8fa8ba,1
    APP_SYMBOL 9,118,442,28,0xc6d9e8,205
    APP_TEXT .Lzorixfs,156,449,0xd9e7f1,1

    /* Main file list column. */
    APP_TEXT .Lpath,316,154,0xf0f7fc,1
    APP_TEXT .Lname_hdr,316,188,0x8fa8ba,1
    APP_TEXT .Lkind_hdr,548,188,0x8fa8ba,1
    APP_ROUND 304,212,624,254,0x64bfff,30,10
    APP_SYMBOL 9,314,218,28,0x83d2ff,238

    movq $0, %rbx
    movabsq $0x00000162000000e2, %r10
    movabsq $0x01ffffff, %r11
    movq $26, %rax
    int $0x80
    APP_TEXT .Ltext_type,548,226,0xc5d9e8,1

    APP_SYMBOL 9,314,267,28,0xbad0df,210
    movq $1, %rbx
    movabsq $0x0000016200000113, %r10
    movabsq $0x01dbe9f3, %r11
    movq $26, %rax
    int $0x80
    APP_TEXT .Llog_type,548,275,0xaec5d6,1

    APP_SYMBOL 9,314,316,28,0xbad0df,210
    movq $2, %rbx
    movabsq $0x0000016200000144, %r10
    movabsq $0x01dbe9f3, %r11
    movq $26, %rax
    int $0x80
    APP_TEXT .Lcfg_type,548,324,0xaec5d6,1

    APP_SYMBOL 9,314,365,28,0xbad0df,210
    movq $3, %rbx
    movabsq $0x0000016200000175, %r10
    movabsq $0x01dbe9f3, %r11
    movq $26, %rax
    int $0x80
    APP_TEXT .Llist_type,548,373,0xaec5d6,1

    APP_SYMBOL 9,314,414,28,0xbad0df,210
    movq $4, %rbx
    movabsq $0x00000162000001a6, %r10
    movabsq $0x01dbe9f3, %r11
    movq $26, %rax
    int $0x80
    APP_TEXT .Ltext_type,548,422,0xaec5d6,1

    /* Fine separators instead of chunky cards. */
    APP_ROUND 304,258,624,259,0xffffff,14,1
    APP_ROUND 304,307,624,308,0xffffff,14,1
    APP_ROUND 304,356,624,357,0xffffff,14,1
    APP_ROUND 304,405,624,406,0xffffff,14,1
    APP_ROUND 304,454,624,455,0xffffff,14,1

    /* Preview column. */
    APP_ROUND 642,146,924,658,0xffffff,8,18
    APP_TEXT .Lpreview_hdr,666,170,0x9bb3c5,1
    APP_SYMBOL 9,748,212,68,0x79caff,232
    APP_TEXT .Lreadme,706,298,0xffffff,1
    APP_TEXT .Ltextdoc,718,326,0x9fb7c8,1
    APP_ROUND 666,364,900,365,0xffffff,15,1
    APP_TEXT .Lcontent,666,386,0x9fb7c8,1

    movq $0, %rbx
    movabsq $0x0000029a000001a2, %r10
    movabsq $0x01e9f5fc, %r11
    movq $29, %rax
    int $0x80

    /* Footer/status line. */
    APP_ROUND 304,624,624,625,0xffffff,13,1
    APP_TEXT .Litems,316,636,0x91aabd,1
    APP_TEXT .Lnativefs,492,636,0x91aabd,1

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
.Lview:        .asciz "VIEW"
.Lsearch:      .asciz "SEARCH"
.Lfavorites:   .asciz "FAVORITES"
.Lhome:        .asciz "HOME"
.Ldesktop:     .asciz "DESKTOP"
.Ldocuments:   .asciz "DOCUMENTS"
.Lsystem:      .asciz "SYSTEM"
.Llocations:   .asciz "LOCATIONS"
.Lzorixfs:     .asciz "ZORIXFS"
.Lpath:        .asciz "ZORIX / HOME"
.Lname_hdr:    .asciz "NAME"
.Lkind_hdr:    .asciz "KIND"
.Ltext_type:   .asciz "TEXT"
.Llog_type:    .asciz "LOG"
.Lcfg_type:    .asciz "CONFIG"
.Llist_type:   .asciz "LIST"
.Lpreview_hdr: .asciz "PREVIEW"
.Lreadme:      .asciz "README.TXT"
.Ltextdoc:     .asciz "TEXT DOCUMENT"
.Lcontent:     .asciz "CONTENTS"
.Litems:       .asciz "5 ITEMS"
.Lnativefs:    .asciz "NATIVE RAMFS"

zk_files_blob_end:
