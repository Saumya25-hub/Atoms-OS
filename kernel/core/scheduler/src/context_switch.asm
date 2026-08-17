[BITS 64]

global context_switch_first

; void context_switch_first(Task* task);
; RDI = Task* task
context_switch_first:
    ; Task->rsp is at offset 32
    mov rsp, [rdi + 32]
    
    ; The pop order MUST remain the exact reverse of isr_stubs.asm.
    ; This routine MUST restore registers exactly as saved.
    ; After restoring GPRs:
    ; add rsp, 16
    ; must skip:
    ; int_no
    ; err_code
    ; before iretq.
    ; This file and isr_stubs.asm are permanently coupled.
    ; Changing one without auditing the other is prohibited.
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    
    ; Drop int_no and err_code
    add rsp, 16
    
    ; Check if target CS is Ring 3 (RPL=3)
    ; [rsp] = RIP, [rsp+8] = CS, [rsp+16] = RFLAGS, [rsp+24] = RSP, [rsp+32] = SS
    test byte [rsp + 8], 3
    jz .kernel_mode
    mov ax, 0x1B
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    iretq

.kernel_mode:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    iretq
