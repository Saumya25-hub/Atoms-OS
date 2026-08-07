[BITS 64]
[GLOBAL _start]
[EXTERN kernel_main]

_start:
    cli
    cld
    mov edx, 0x3F8
    mov al, '!'
    out dx, al
    mov al, 'K'
    out dx, al
    mov al, 'E'
    out dx, al
    mov al, 'R'
    out dx, al
    mov al, 'N'
    out dx, al
    mov al, 'E'
    out dx, al
    mov al, 'L'
    out dx, al
    mov al, 10
    out dx, al

    mov rsp, 0x90000
    and rsp, 0xFFFFFFFFFFFFFFF0

    ; Enable SSE / AVX support in CR0 & CR4
    mov rax, cr0
    and ax, 0xFFFB      ; Clear EM (bit 2)
    or ax, 0x0002       ; Set MP (bit 1)
    mov cr0, rax

    mov rax, cr4
    or eax, 0x600       ; Set OSFXSR (bit 9) & OSXMMEXCPT (bit 10)
    mov cr4, rax

    ; Preserve RDI (contains boot_info pointer from bootloader)
    push rdi

    ; Execute the C Kernel
    call kernel_main
    
    ; The PM explicitly ordered: After kernel_main returns, THEN execute cli/hlt.
.end:
    cli
    hlt
    jmp .end
