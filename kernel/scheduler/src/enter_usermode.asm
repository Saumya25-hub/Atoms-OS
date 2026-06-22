[BITS 64]

global enter_usermode

; -----------------------------------
; void enter_usermode(uint64_t entry_point, uint64_t user_stack);
; -----------------------------------
; RDI = user code entry point
; RSI = user stack pointer
enter_usermode:
    cli                     ; Disable interrupts

    ; Load User Data Segment into DS, ES, FS, GS
    ; To prevent iretq from nullifying them and potentially causing a hypervisor bug
    mov ax, 0x1B            ; 0x18 (User Data) | 3 (RPL)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Prepare IRETQ frame for Ring 3 transition
    
    ; 1. Push SS (User Data Segment: 0x1B)
    mov rax, 0x18           ; 0x18 is GDT_USER_DATA
    or rax, 3               ; RPL = 3
    push rax

    ; 2. Push RSP (User Stack)
    push rsi

    ; 3. Push RFLAGS
    ; We MUST HAVE IF (Interrupt Enable) = 0 in User Mode for Sprint 8.
    ; Without a TSS, a timer interrupt in Ring 3 will cause a Triple Fault.
    pushf
    pop rax
    and rax, ~0x200         ; Clear IF
    push rax

    ; 4. Push CS (User Code Segment: 0x23)
    mov rax, 0x20           ; 0x20 is GDT_USER_CODE
    or rax, 3               ; RPL = 3
    push rax

    ; 5. Push RIP (User Entry Point)
    push rdi

    ; Execute IRETQ to transition to Ring 3!
    iretq
