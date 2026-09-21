; =============================================================================
; ATOMS OS — Intel VT-x (VMX) Low-Level Guest Context Switcher
; Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
;
; Phase 4: Full FreeBSD amd64 Hardware Virtualization Execution Loop
; =============================================================================

[BITS 64]
DEFAULT REL

section .text

global vmx_run_vcpu_raw
global vmx_vmexit_handler
global g_vmlaunch_rflags
global g_vmlaunch_class

; Pointer to current host-active GuestCpuRegisters struct
section .bss
align 8
g_active_guest_regs: resq 1
g_saved_host_rsp:    resq 1
g_vmlaunch_rflags:   resq 1
g_vmlaunch_class:    resq 1

section .text

; -----------------------------------------------------------------------------
; bool vmx_run_vcpu_raw(GuestCpuRegisters *regs, bool is_resuming)
; RDI = pointer to GuestCpuRegisters (GPA/HVA struct)
; RSI = 0 for vmlaunch (first entry), 1 for vmresume (subsequent entry)
; Returns: true (1) if VM-exit occurred, false (0) if vmlaunch/vmresume failed.
; -----------------------------------------------------------------------------
vmx_run_vcpu_raw:
    ; Save host callee-saved registers
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15
    push rdi        ; save pointer to regs

    ; Store pointer in global for vmexit handler
    mov [g_active_guest_regs], rdi
    mov [g_saved_host_rsp], rsp
    mov qword [g_vmlaunch_rflags], 0
    mov qword [g_vmlaunch_class], 0

    ; Load guest general purpose registers from regs struct
    mov rax, [rdi + 0x00]   ; rax
    mov rbx, [rdi + 0x08]   ; rbx
    mov rcx, [rdi + 0x10]   ; rcx
    mov rdx, [rdi + 0x18]   ; rdx
    mov r8,  [rdi + 0x40]   ; r8
    mov r9,  [rdi + 0x48]   ; r9
    mov r10, [rdi + 0x50]   ; r10
    mov r11, [rdi + 0x58]   ; r11
    mov r12, [rdi + 0x60]   ; r12
    mov r13, [rdi + 0x68]   ; r13
    mov r14, [rdi + 0x70]   ; r14
    mov r15, [rdi + 0x78]   ; r15
    mov rbp, [rdi + 0x30]   ; rbp

    ; Determine whether to vmlaunch or vmresume
    cmp rsi, 0
    jne .do_resume

.do_launch:
    mov rsi, [rdi + 0x20]   ; rsi
    mov rdi, [rdi + 0x28]   ; rdi (last!)
    vmlaunch
    pushfq
    pop rax
    mov [g_vmlaunch_rflags], rax
    jc .set_vmfail_invalid
    jz .set_vmfail_valid
    mov qword [g_vmlaunch_class], 3
    jmp .launch_failed

.do_resume:
    mov rsi, [rdi + 0x20]   ; rsi
    mov rdi, [rdi + 0x28]   ; rdi (last!)
    vmresume
    pushfq
    pop rax
    mov [g_vmlaunch_rflags], rax
    jc .set_vmfail_invalid
    jz .set_vmfail_valid
    mov qword [g_vmlaunch_class], 3
    jmp .launch_failed

.set_vmfail_invalid:
    mov qword [g_vmlaunch_class], 1 ; VMfailInvalid (CF=1)
    jmp .launch_failed

.set_vmfail_valid:
    mov qword [g_vmlaunch_class], 2 ; VMfailValid (ZF=1)
    jmp .launch_failed

.launch_failed:
    ; If vmlaunch or vmresume returns, an error occurred
    mov rsp, [g_saved_host_rsp]
    pop rdi
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx
    xor eax, eax            ; return false (0)
    ret

; -----------------------------------------------------------------------------
; vmx_vmexit_handler (Host RIP loaded by CPU upon VM-exit)
; -----------------------------------------------------------------------------
align 16
vmx_vmexit_handler:
    ; CPU automatically loaded Host CR0, CR3, CR4, Host RSP, and jumped here.
    mov qword [g_vmlaunch_class], 0  ; Class 0 = Architectural VM-exit taken
    mov qword [g_vmlaunch_rflags], 2 ; Architectural host RFLAGS
    ; First save guest RAX & RDI temporarily on stack to get address of struct
    push rdi
    push rax

    mov rdi, [g_active_guest_regs]
    test rdi, rdi
    jz .emergency_recovery

    ; Pop saved guest RAX and RDI into struct
    pop rax
    mov [rdi + 0x00], rax   ; guest rax
    pop rax
    mov [rdi + 0x28], rax   ; guest rdi

    ; Save all remaining guest GPRs into struct
    mov [rdi + 0x08], rbx   ; rbx
    mov [rdi + 0x10], rcx   ; rcx
    mov [rdi + 0x18], rdx   ; rdx
    mov [rdi + 0x20], rsi   ; rsi
    mov [rdi + 0x30], rbp   ; rbp
    mov [rdi + 0x40], r8    ; r8
    mov [rdi + 0x48], r9    ; r9
    mov [rdi + 0x50], r10   ; r10
    mov [rdi + 0x58], r11   ; r11
    mov [rdi + 0x60], r12   ; r12
    mov [rdi + 0x68], r13   ; r13
    mov [rdi + 0x70], r14   ; r14
    mov [rdi + 0x78], r15   ; r15

    ; Restore Host RSP and callee-saved registers
    mov rsp, [g_saved_host_rsp]
    pop rdi
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx

    mov eax, 1              ; return true (1) -> VM exit handled
    ret

.emergency_recovery:
    pop rax
    pop rdi
    mov rsp, [g_saved_host_rsp]
    pop rdi
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx
    xor eax, eax
    ret
