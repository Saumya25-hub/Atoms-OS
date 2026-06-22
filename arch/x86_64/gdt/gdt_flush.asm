[BITS 64]

global gdt_flush
global tss_flush

; -----------------------------------
; void gdt_flush(uint64_t gdtr_ptr)
; -----------------------------------
gdt_flush:
    ; RDI contains the pointer to the GDTR
    lgdt [rdi]

    ; Load data segment registers with Kernel Data Selector (0x10)
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Reload CS with Kernel Code Selector (0x08)
    ; In 64-bit mode, we can't use far jmp directly like in 32-bit mode.
    ; We must push the selector and the return address, then use lretq.
    pop rdi          ; Pop the return address
    push 0x08        ; Push the new CS
    push rdi         ; Push the return address back
    retfq            ; Return far to reload CS

; -----------------------------------
; void tss_flush(void)
; -----------------------------------
tss_flush:
    ; Load the Task Register with the TSS Selector
    ; The TSS starts at offset 5 in the GDT, so 5 * 8 = 40 = 0x28
    mov ax, 0x28
    ltr ax
    ret
