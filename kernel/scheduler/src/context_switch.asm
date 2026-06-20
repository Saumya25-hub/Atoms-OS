[BITS 64]

global context_switch_first

; void context_switch_first(Task* task);
; RDI = Task* task
context_switch_first:
    ; Task->rsp is at offset 32
    mov rsp, [rdi + 32]
    
    ; Pop General Purpose Registers from Context structure
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    pop rdx
    pop rcx
    pop rax
    
    ; Now RSP points to the interrupt frame (RIP, CS, RFLAGS, RSP, SS)
    ; Perform ONE transition into TaskA using iretq
    iretq
