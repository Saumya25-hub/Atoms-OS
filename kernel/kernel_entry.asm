[BITS 64]
[GLOBAL _start]
[EXTERN kernel_main]

; ==========================================================================
; Kernel BSP Boot Stack (16KB, 16-byte aligned, in .bss section)
; Allocated inside the kernel image — UEFI AllocatePages guarantees ownership.
; Linux/FreeBSD do the same: stack lives in .bss, NOT at arbitrary low memory.
;
; WHY NOT 0x90000:
;   On real UEFI H81 hardware, 0x80000-0x9FFFF is EBDA / UEFI Runtime /
;   SMM reserved memory. The UEFI bootloader never called AllocatePages for
;   this range, so firmware may read/write it at any time via SMI handlers,
;   corrupting the stack and causing silent hangs on function return (retq).
;   QEMU marks this region as conventional memory, masking the bug.
; ==========================================================================
section .bss
align 16
boot_stack_bottom:
    resb 16384              ; 16KB BSP kernel boot stack
boot_stack_top:

section .text

_start:
    cli
    cld

    ; Validate boot_info pointer in RDI
    test rdi, rdi
    jz .halt

    ; Initialize kernel stack in .bss (UEFI-allocated, firmware-safe memory)
    lea rsp, [rel boot_stack_top]
    and rsp, 0xFFFFFFFFFFFFFFF0      ; Enforce 16-byte ABI alignment

    ; Enable SSE support in CR0 & CR4
    mov rax, cr0
    and ax, 0xFFFB          ; Clear EM (bit 2)
    or ax, 0x0002           ; Set MP (bit 1)
    mov cr0, rax

    mov rax, cr4
    or eax, 0x600           ; Set OSFXSR (bit 9) & OSXMMEXCPT (bit 10)
    mov cr4, rax

    ; Zero RBP for clean stack trace termination (matches Linux head_64.S)
    xor rbp, rbp

    ; Jump to C kernel_main with RDI = boot_info
    call kernel_main

.halt:
    cli
    hlt
    jmp .halt
