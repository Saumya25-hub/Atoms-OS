[BITS 64]

global enter_usermode

; -----------------------------------
; void enter_usermode(uint64_t entry_point, uint64_t user_stack);
; -----------------------------------
; RDI = user code entry point
; RSI = user stack pointer
enter_usermode:
    cli                     ; Disable interrupts

    ; Prepare IRETQ frame for Ring 3 transition
    
    ; 1. Push SS (User Data Segment: 0x1B)
    mov rax, 0x18           ; 0x18 is GDT_USER_DATA
    or rax, 3               ; RPL = 3
    push rax

    ; 2. Push RSP (User Stack)
    push rsi

    ; 3. Push RFLAGS
    ; We want IF (Interrupt Enable) = 1 in User Mode.
    ; RFLAGS IF is bit 9 (0x200). 
    ; Let's get current RFLAGS, set bit 9, and push it.
    pushf
    pop rax
    or rax, 0x200           ; Set IF
    push rax

    ; 4. Push CS (User Code Segment: 0x23)
    mov rax, 0x20           ; 0x20 is GDT_USER_CODE
    or rax, 3               ; RPL = 3
    push rax

    ; 5. Push RIP (User Entry Point)
    push rdi

    ; Execute IRETQ to transition to Ring 3!
    iretq
