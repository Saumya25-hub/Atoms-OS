[BITS 64]

global syscall_init_asm
global syscall_entry
global syscall_kernel_stack
extern syscall_handler

section .data
align 8
syscall_kernel_stack dq 0
syscall_user_stack   dq 0

section .text

; -----------------------------------------------------------------------------
; syscall_init_asm: Configures the MSRs for the SYSCALL/SYSRET instructions
; -----------------------------------------------------------------------------
syscall_init_asm:
    push rbp
    mov rbp, rsp

    ; 1. Enable System Call Extensions (SCE) in IA32_EFER MSR (0xC0000080)
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1           ; Set SCE bit (Bit 0)
    wrmsr

    ; 2. Setup CS/SS bases in IA32_STAR MSR (0xC0000081)
    ; Syscall loads Kernel CS from STAR[47:32] and SS from STAR[47:32]+8
    ; Sysret loads User CS from STAR[63:48]+16 and SS from STAR[63:48]+8
    ; Our GDT:
    ; 0x08 = Kernel Code
    ; 0x10 = Kernel Data
    ; 0x18 = User Data
    ; 0x20 = User Code
    ; So, STAR[47:32] = 0x08 (Kernel Code)
    ; STAR[63:48] = 0x10 (Sysret adds 16 -> 0x20 User Code, adds 8 -> 0x18 User Data)
    mov ecx, 0xC0000081
    rdmsr
    mov edx, 0x00100008 ; EDX is high 32 bits of MSR (STAR[63:32])
    mov eax, 0          ; EAX is low 32 bits
    wrmsr

    ; 3. Setup Target RIP in IA32_LSTAR MSR (0xC0000082)
    mov ecx, 0xC0000082
    mov rax, syscall_entry
    mov edx, eax        ; Low 32 bits
    shr rax, 32         
    ; Wait! EAX is low 32 bits, EDX is high 32 bits. Let's do it right.
    mov rax, syscall_entry
    mov edx, eax
    shr rax, 32
    xchg eax, edx       ; Now EAX=low 32, EDX=high 32
    wrmsr

    ; 4. Setup IA32_FMASK MSR (0xC0000084)
    mov ecx, 0xC0000084
    rdmsr
    mov eax, 0x00000200 ; Mask out Interrupt Flag (IF) during syscall
    mov edx, 0
    wrmsr

    pop rbp
    ret

; -----------------------------------------------------------------------------
; syscall_entry: Entry point for the SYSCALL instruction
; CPU State on entry:
; RIP = syscall_entry
; RCX = User RIP
; R11 = User RFLAGS
; CS  = Kernel Code (0x08)
; SS  = Kernel Data (0x10)
; RSP = STILL USER RSP! (We must swap to a kernel stack)
; -----------------------------------------------------------------------------
align 16
syscall_entry:
    ; 1. Swap stack to kernel stack
    mov [rel syscall_user_stack], rsp
    mov rsp, [rel syscall_kernel_stack]

    ; 2. Push state to create a standard frame for C handler
    push r11            ; User RFLAGS
    push rcx            ; User RIP
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15

    ; SysV ABI passes syscall arguments in:
    ; RAX (Syscall ID)
    ; RDI, RSI, RDX, R10, R8, R9
    ; Note: RCX is used for RIP, so R10 is used for the 4th argument instead.
    
    ; Call the C handler
    ; We already have the arguments in the right registers (mostly).
    ; C handler signature: uint64_t syscall_handler(uint64_t id, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6);
    ; But standard SysV puts:
    ; RDI = id (from RAX)
    ; RSI = arg1 (from RDI)
    ; RDX = arg2 (from RSI)
    ; RCX = arg3 (from RDX)
    ; R8  = arg4 (from R10)
    ; R9  = arg5 (from R8)
    ; Stack = arg6 (from R9)
    ; Instead of shifting everything, let's just pass a pointer to a struct, 
    ; or simply let the C handler take standard parameters if we move them!

    ; Move parameters to match C SysV ABI:
    push r9             ; Save arg6
    mov r9, r8          ; arg5
    mov r8, r10         ; arg4
    mov rcx, rdx        ; arg3
    mov rdx, rsi        ; arg2
    mov rsi, rdi        ; arg1
    mov rdi, rax        ; syscall_id

    ; Align stack to 16 bytes before call
    ; We pushed 9 registers (72 bytes). 72 % 16 = 8.
    ; We need to push 8 more bytes to make it 80 bytes (16-byte aligned).
    sub rsp, 8

    call syscall_handler

    ; Restore stack alignment without destroying RAX (which holds the return value!)
    add rsp, 8
    pop r9              ; Restore arg6 just to balance stack

    ; 3. Restore state
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx
    pop rcx             ; User RIP
    pop r11             ; User RFLAGS

    ; 4. Swap stack back to user stack
    mov rsp, [rel syscall_user_stack]

    ; 5. Return to userspace
    ; SYSRET requires RCX=RIP, R11=RFLAGS, and returns to Ring 3.
    ; NOTE: 64-bit sysret requires 'sysretq' or 'o64 sysret' depending on assembler.
    ; NASM uses 'o64 sysret' for 64-bit.
    o64 sysret
