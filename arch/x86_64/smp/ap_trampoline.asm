; ============================================================
; ATOMS OS — AP Real-Mode to 64-Bit Long Mode Trampoline
; Location at Runtime: Physical Address 0x8000 (Vector 0x08)
; ============================================================

[BITS 16]
DEFAULT ABS

global ap_trampoline_start
global ap_trampoline_end
global ap_trampoline_cpuid
global ap_trampoline_stack

extern ap_main

ap_trampoline_start:
    cli
    cld

    ; Set up 16-bit segments (CS is 0x0800 when SIPI vector 0x08 executes)
    mov ax, 0x0800
    mov ds, ax
    mov ss, ax
    mov sp, 0x7C00

    ; Load temporary 32-bit GDT (offset relative to 0x8000)
    lgdt [ds:ap_gdt_desc - ap_trampoline_start]

    ; Enable Protected Mode (CR0.PE = 1)
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    ; Far jump to 32-bit protected mode code segment (0x08)
    jmp dword 0x08:(0x8000 + (ap_trampoline_32 - ap_trampoline_start))

[BITS 32]
ap_trampoline_32:
    ; Set 32-bit data segment registers (0x10)
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Enable Physical Address Extension (CR4.PAE = 1)
    mov eax, cr4
    or eax, 0x20
    mov cr4, eax

    ; Load PML4 physical page table address into CR3 (0x10000)
    mov eax, 0x10000
    mov cr3, eax

    ; Enable Long Mode (IA32_EFER.LME = 1)
    mov ecx, 0xC0000080
    rdmsr
    or eax, 0x00000100
    wrmsr

    ; Enable Paging (CR0.PG = 1)
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax

    ; Far jump to 64-bit long mode code segment (0x18)
    jmp dword 0x18:(0x8000 + (ap_trampoline_64 - ap_trampoline_start))

[BITS 64]
ap_trampoline_64:
    ; Set 64-bit data segment registers (0x20)
    mov ax, 0x20
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Load per-CPU kernel stack pointer from mailbox (0x8000 + stack offset)
    mov rax, qword [0x8000 + (ap_trampoline_stack - ap_trampoline_start)]
    mov rsp, rax

    ; Load CPU logical ID into RDI from mailbox (0x8000 + cpuid offset)
    mov edi, dword [0x8000 + (ap_trampoline_cpuid - ap_trampoline_start)]

    ; Align stack to 16 bytes for C ABI compliance
    and rsp, -16

    ; Jump to C entry point ap_main(logical_id)
    mov rax, ap_main
    call rax

.ap_halt:
    hlt
    jmp .ap_halt

align 16
ap_gdt:
    ; 0x00: Null Descriptor
    dq 0x0000000000000000
    ; 0x08: 32-bit Code Segment
    dq 0x00CF9A000000FFFF
    ; 0x10: 32-bit Data Segment
    dq 0x00CF92000000FFFF
    ; 0x18: 64-bit Code Segment
    dq 0x00AF9A000000FFFF
    ; 0x20: 64-bit Data Segment
    dq 0x00AF92000000FFFF
ap_gdt_end:

align 4
ap_gdt_desc:
    dw (ap_gdt_end - ap_gdt - 1)
    dd (0x8000 + (ap_gdt - ap_trampoline_start))

align 8
ap_trampoline_cpuid:
    dd 0

align 8
ap_trampoline_stack:
    dq 0

ap_trampoline_end:
