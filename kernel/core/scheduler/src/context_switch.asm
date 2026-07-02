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
    
    ; Now RSP points to the interrupt frame (RIP, CS, RFLAGS, RSP, SS)
    ; Perform ONE transition into TaskA using iretq
    iretq
