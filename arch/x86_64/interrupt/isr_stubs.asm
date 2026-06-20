[BITS 64]

extern isr_common_handler

%macro ISR_NOERRCODE 1
global isr%1
isr%1:
    push 0      ; Push dummy error code
    push %1     ; Push interrupt number
    jmp isr_common_stub
%endmacro

%macro ISR_ERRCODE 1
global isr%1
isr%1:
    push %1     ; Push interrupt number
    jmp isr_common_stub
%endmacro

; 0: Divide By Zero Exception
ISR_NOERRCODE 0
; 1: Debug Exception
ISR_NOERRCODE 1
; 2: Non Maskable Interrupt Exception
ISR_NOERRCODE 2
; 3: Int 3 Exception
ISR_NOERRCODE 3
; 4: INTO Exception
ISR_NOERRCODE 4
; 5: Out of Bounds Exception
ISR_NOERRCODE 5
; 6: Invalid Opcode Exception
ISR_NOERRCODE 6
; 7: Coprocessor Not Available Exception
ISR_NOERRCODE 7
; 8: Double Fault Exception (With Error Code!)
ISR_ERRCODE   8
; 9: Coprocessor Segment Overrun Exception
ISR_NOERRCODE 9
; 10: Bad TSS Exception (With Error Code!)
ISR_ERRCODE   10
; 11: Segment Not Present Exception (With Error Code!)
ISR_ERRCODE   11
; 12: Stack Fault Exception (With Error Code!)
ISR_ERRCODE   12
; 13: General Protection Fault Exception (With Error Code!)
ISR_ERRCODE   13
; 14: Page Fault Exception (With Error Code!)
ISR_ERRCODE   14
; 15: Reserved Exception
ISR_NOERRCODE 15
; 16: Floating Point Exception
ISR_NOERRCODE 16
; 17: Alignment Check Exception
ISR_ERRCODE   17
; 18: Machine Check Exception
ISR_NOERRCODE 18
; 19: SIMD Floating Point Exception
ISR_NOERRCODE 19
; 20: Virtualization Exception
ISR_NOERRCODE 20
; 21: Control Protection Exception
ISR_ERRCODE   21
; 22-31: Reserved
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_ERRCODE   30
ISR_NOERRCODE 31

; IRQs 32-255
%assign i 32
%rep 224
    ISR_NOERRCODE i
%assign i i+1
%endrep

isr_common_stub:
    ; Save general purpose registers
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    ; The C handler takes a pointer to the registers struct.
    ; In x86_64 System V ABI, the first argument is passed in RDI.
    ; RSP currently points to the top of our registers_t struct.
    mov rdi, rsp

    ; Call the C generic handler
    call isr_common_handler

    ; Restore general purpose registers
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

; Generate the table of function pointers
global isr_stub_table
isr_stub_table:
%assign i 0
%rep 256
    dq isr%+i
%assign i i+1
%endrep
