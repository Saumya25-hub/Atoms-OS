global enter_usermode

; void enter_usermode(uint64_t rip, uint64_t rsp);
; rdi = rip
; rsi = rsp
enter_usermode:
    cli

    mov ax, 0x23 ; User Data Segment (0x20) | DPL 3 (0x3) = 0x23
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Stack frame for iretq: SS, RSP, RFLAGS, CS, RIP
    push 0x23    ; SS
    push rsi     ; RSP
    push 0x202   ; RFLAGS (Interrupts Enabled: IF=1)
    push 0x1B    ; CS (User Code Segment (0x18) | DPL 3 (0x3) = 0x1B)
    push rdi     ; RIP

    iretq
