[BITS 64]

global enter_usermode
global phase_b_jump_usermode

enter_usermode:
    cli                     ; Disable interrupts

    mov ax, 0x1B            ; 0x18 (User Data) | 3 (RPL)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov rax, 0x18           ; 0x18 is GDT_USER_DATA
    or rax, 3               ; RPL = 3
    push rax

    push rsi                ; User Stack Pointer

    pushf
    pop rax
    and rax, ~0x200         ; Clear IF
    push rax

    mov rax, 0x20           ; 0x20 is GDT_USER_CODE
    or rax, 3               ; RPL = 3
    push rax

    push rdi                ; User Entry Point

    iretq

phase_b_jump_usermode:
    mov rsp, rdx
    cli
    mov ax, 0x1B
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov rax, 0x18
    or rax, 3
    push rax

    push rsi

    pushf
    pop rax
    and rax, ~0x200
    push rax

    mov rax, 0x20
    or rax, 3
    push rax

    push rdi

    iretq
