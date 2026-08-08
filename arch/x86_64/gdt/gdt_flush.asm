[BITS 64]

global gdt_flush
global tss_flush

; -----------------------------------
; void gdt_flush(uint64_t gdtr_ptr)
; -----------------------------------
gdt_flush:
    lgdt [rdi]

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Reload CS segment (0x08) cleanly via far return in 64-bit NASM
    push qword 0x08
    lea rax, [rel reload_cs]
    push rax
    retfq

reload_cs:
    ret

; -----------------------------------
; void tss_flush(void)
; -----------------------------------
tss_flush:
    mov ax, 0x28
    ltr ax
    ret
