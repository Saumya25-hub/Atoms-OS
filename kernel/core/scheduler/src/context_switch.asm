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
    ; Set segment registers BEFORE restoring GPRs so RAX is never clobbered
    test byte [rsp + 144], 3 ; CS is at offset 15*8 + 16 + 8 = 144
    jz .kernel_segments
    mov ax, 0x1B
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    jmp .restore_gprs

.kernel_segments:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

.restore_gprs:
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
    
    iretq
