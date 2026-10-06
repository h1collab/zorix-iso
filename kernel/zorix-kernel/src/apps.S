.text
.globl zk_settings_blob_start
.globl zk_settings_blob_end
.globl zk_files_blob_start
.globl zk_files_blob_end

/* Small position-independent native apps. They execute at CPL3, verify their
 * PID through the native syscall surface, announce readiness, then exit. */

zk_settings_blob_start:
    movq $0, %rax
    int $0x80
    cmpq $100, %rax
    jne .Lsettings_fail
    movq $1, %rbx
    movq $16, %rax
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
zk_settings_blob_end:

zk_files_blob_start:
    movq $0, %rax
    int $0x80
    cmpq $101, %rax
    jne .Lfiles_fail
    movq $2, %rbx
    movq $16, %rax
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
zk_files_blob_end:
