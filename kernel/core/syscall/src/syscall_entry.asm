[BITS 64]

global syscall_init_asm
global syscall_entry
extern syscall_handler
extern syscall_prepare_return
extern scheduler_yield
extern tss

%define TSS_RSP0                 4
%define TSS_SYSCALL_USER_RSP     104
%define TSS_SYSCALL_FRAME        112
%define TSS_SYSCALL_NESTING      120

%define FRAME_USER_RSP           0
%define FRAME_USER_RIP           8
%define FRAME_USER_RFLAGS        16
%define FRAME_NUMBER             24
%define FRAME_ARG1               32
%define FRAME_ARG2               40
%define FRAME_ARG3               48
%define FRAME_ARG4               56
%define FRAME_ARG5               64
%define FRAME_ARG6               72
%define FRAME_RESULT             80
%define FRAME_NESTING            108
%define FRAME_SIZE               112

%define RETURN_SYSRET            0
%define RETURN_IRET              1

section .text

syscall_init_asm:
    push rbp
    mov rbp, rsp

    mov ecx, 0xC0000080          ; IA32_EFER
    rdmsr
    or eax, 1                    ; SCE
    wrmsr

    mov ecx, 0xC0000081          ; IA32_STAR
    mov edx, 0x00100008          ; SYSRET base 0x10, SYSCALL CS 0x08
    xor eax, eax
    wrmsr

    mov ecx, 0xC0000082          ; IA32_LSTAR
    mov rax, syscall_entry
    mov rdx, rax
    shr rdx, 32
    wrmsr

    mov ecx, 0xC0000084          ; IA32_FMASK
    ; Clear TF, IF, DF, IOPL, NT, RF, AC on entry. Return policy validates again.
    mov eax, 0x00077F00
    xor edx, edx
    wrmsr

    pop rbp
    ret

align 16
syscall_entry:
    mov [rel tss + TSS_SYSCALL_USER_RSP], rsp
    mov rsp, [rel tss + TSS_RSP0]

    ; Preserve SysV callee-saved registers outside the public syscall frame.
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15
    sub rsp, FRAME_SIZE

    mov [rsp + FRAME_USER_RIP], rcx
    mov [rsp + FRAME_USER_RFLAGS], r11
    mov [rsp + FRAME_NUMBER], rax
    mov [rsp + FRAME_ARG1], rdi
    mov [rsp + FRAME_ARG2], rsi
    mov [rsp + FRAME_ARG3], rdx
    mov [rsp + FRAME_ARG4], r10
    mov [rsp + FRAME_ARG5], r8
    mov [rsp + FRAME_ARG6], r9
    mov rax, [rel tss + TSS_SYSCALL_USER_RSP]
    mov [rsp + FRAME_USER_RSP], rax

    inc dword [rel tss + TSS_SYSCALL_NESTING]
    mov eax, [rel tss + TSS_SYSCALL_NESTING]
    mov [rsp + FRAME_NESTING], ax
    mov [rel tss + TSS_SYSCALL_FRAME], rsp

    mov rdi, rsp
    call syscall_handler
    mov [rsp + FRAME_RESULT], rax

    cli                           ; Ensure return preparation & frame tear-down is strictly atomic

    mov rdi, rsp
    call syscall_prepare_return
    mov r10, rax                  ; validated return mode
    mov rax, [rsp + FRAME_RESULT]
    mov rcx, [rsp + FRAME_USER_RIP]
    mov r11, [rsp + FRAME_USER_RFLAGS]
    mov r9, [rsp + FRAME_USER_RSP]

    mov qword [rel tss + TSS_SYSCALL_FRAME], 0
    dec dword [rel tss + TSS_SYSCALL_NESTING]

    add rsp, FRAME_SIZE
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx

    cmp r10, RETURN_SYSRET
    je .return_sysret
    cmp r10, RETURN_IRET
    je .return_iret

.safe_failure:
    ; C marked the unsafe/current task terminated. Never execute SYSRET with
    ; rejected state; yield and remain halted if no runnable replacement exists.
    call scheduler_yield
    sti
.halt_rejected:
    hlt
    jmp .halt_rejected

.return_iret:
    mov r8w, 0x1B
    mov ds, r8w
    mov es, r8w
    push qword 0x1B               ; user SS
    push r9                       ; user RSP
    push r11                      ; sanitized RFLAGS
    push qword 0x23               ; user CS
    push rcx                      ; user RIP
    iretq

.return_sysret:
    mov r8w, 0x1B
    mov ds, r8w
    mov es, r8w
    mov rsp, r9
    o64 sysret
