[BITS 64]
[GLOBAL _start]
[EXTERN kernel_main]

_start:
    ; Align the stack to 16 bytes per System V ABI requirements
    and rsp, 0xFFFFFFFFFFFFFFF0
    
    ; Disable interrupts explicitly during kernel initialization
    cli
    
    ; Clear Direction Flag to ensure string operations go forward
    cld
    
    ; Preserve RDI (contains boot_info pointer from bootloader)
    push rdi

    ; Zero out the BSS section
    extern _bss_start
    extern _bss_end
    mov rdi, _bss_start
    mov rcx, _bss_end
    sub rcx, rdi
    xor al, al
    rep stosb

    ; Restore RDI (boot_info pointer)
    pop rdi

    ; Execute the C Kernel
    call kernel_main
    
    ; The PM explicitly ordered: After kernel_main returns, THEN execute cli/hlt.
.end:
    cli
    hlt
    jmp .end
