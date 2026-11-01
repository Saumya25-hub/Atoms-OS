# AUDIT PART 2: VMX CORE ARCHITECTURE & DISPATCHER

## 1. VMXON Region & Initialization

- **Allocation**:
  - [VERIFIED-BY-CODE: kernel/core/hypervisor/src/hypervisor.c:2845-2865]
  - Size: 4,096 bytes (1 physical page).
  - Revision ID: Read from MSR 0x480 (IA32_VMX_BASIC), written to offset 0.
- **Enabling**:
  - [VERIFIED-BY-CODE: kernel/core/hypervisor/src/hypervisor.c:2855-2858]
  - CPU CR4 bit 13 (CR4_VMXE) set via assembly.
  - Software executes __vmx_on with physical address of VMXON page.
  - SDM Reference: Intel SDM Vol 3C, Section 24.1.5 ('Enabling and Entering VMX Operation').

## 2. VMCS Field Configuration & Intel SDM Compliance

### 2.1 Execution Controls & MSR Clamping
- [VERIFIED-BY-CODE: kernel/core/hypervisor/src/hypervisor.c:3370-3401]
- **MSR Clamping Function**: vmx_adjust_controls(desired, msr_index)
  - Reads capability MSR (e.g. IA32_VMX_TRUE_PINBASED_CTLS 0x48D or IA32_VMX_PINBASED_CTLS 0x481).
  - Forces bits: (desired | (uint32_t)msr_val) & (uint32_t)(msr_val >> 32).
  - Ensures no unsupported bit is set and all mandatory 1-bits are populated.
- **Fields Configured**:
  1. VMCS_PIN_BASED_VM_EXEC_CONTROL (0x4000): External interrupt exiting = 0, NMI exiting = 0, Virtual NMIs = 0.
  2. VMCS_CPU_BASED_VM_EXEC_CONTROL (0x4002):
     - HLT exiting (bit 7) = 1 (Trap guest HLT to host cooperative scheduler).
     - CR3 load/store exiting (bits 15, 16) = 0 (FreeBSD manages guest paging directly without exit).
     - Unconditional I/O exiting (bit 24) = 1 (All Port I/O traps to virtual_platform_emulate_io).
     - Use MSR bitmaps (bit 28) = 0.
     - Interrupt-window exiting (bit 2) = dynamic (Enabled only when an interrupt is pending but guest IF = 0).
  3. VMCS_SECONDARY_VM_EXEC_CONTROL (0x401E):
     - Enable EPT (bit 1) = 1.
     - Enable RDTSCP (bit 3) = 1.
     - Enable VPID (bit 5) = 0 (VPID not active).
     - Unrestricted guest (bit 7) = 1 (Silicon permitting).
  4. VMCS_VM_EXIT_CONTROLS (0x400C):
     - Host address-space size (bit 9) = 1 (Host returns in 64-bit Long Mode).
  5. VMCS_VM_ENTRY_CONTROLS (0x4012):
     - IA-32e mode guest (bit 9) = 1 (Guest boots directly in 64-bit Long Mode).

### 2.2 Host State Configuration
- [VERIFIED-BY-CODE: kernel/core/hypervisor/src/hypervisor.c:3305-3325]
- HOST_CR0: Host CR0 (Paging + Protected Mode).
- HOST_CR3: Host CR3 (vmm_get_kernel_cr3()).
- HOST_CR4: Host CR4.
- HOST_CS_SELECTOR: 0x08 (Kernel Code).
- HOST_SS_SELECTOR, DS, ES, FS, GS: 0x10 (Kernel Data).
- HOST_TR_SELECTOR: Current host TSS selector (0x28).
- HOST_GDTR_BASE: Physical base of host GDT.
- HOST_IDTR_BASE: Physical base of host IDT.
- HOST_RSP: Dedicated host VM-exit stack (vcpu->host_stack_top).
- HOST_RIP: (uint64_t)vmx_vmexit_handler (Entrypoint in vmx_entry.asm).

### 2.3 Guest State Configuration
- [VERIFIED-BY-CODE: kernel/core/hypervisor/src/hypervisor.c:3165-3278]
- GUEST_CR0: 0x80010031 (PE=1, MP=1, ET=1, NE=1, WP=1, PG=1).
- GUEST_CR3: 0x20000 (FreeBSD guest PML4 constructed by freebsd_loader_setup_guest_paging).
- GUEST_CR4: 0x000026A0 (VME=0, PVI=0, TSD=0, DE=1, PSE=0, PAE=1, MCE=0, PGE=1, PCE=0, OSFXSR=1, OSXMMEXCPT=1, VMXE=0, FSGSBASE=1).
- GUEST_IA32_EFER: 0x00000D01 (SCE=1, LME=1, LMA=1, NXE=1).
- GUEST_RIP: 0xFFFFFFFF8037C000 (FreeBSD kernel ELF btext entrypoint).
- GUEST_RSP: 0x000000000007FF00.
- GUEST_RFLAGS: 0x00000002 (Bit 1 reserved = 1, IF = 0 at boot).
- GUEST_CS_SELECTOR: 0x08, Limit 0xFFFFFFFF, AR: 0x0000A09B (Code, 64-bit Long Mode, Present, DPL 0).
- GUEST_SS_SELECTOR, DS, ES, FS, GS: 0x10, Limit 0xFFFFFFFF, AR: 0x0000C093 (Data Read/Write, 4GB limit, Present, DPL 0).
- GUEST_TR_SELECTOR: 0x28, Limit 0x67, AR: 0x0000008B (64-bit TSS Busy).
- GUEST_LDTR_SELECTOR: 0x00, Limit 0x00, AR: 0x00010000 (Unusable).
- GUEST_GDTR_BASE: Host GDT base, Limit 0xFFFF.
- GUEST_IDTR_BASE: GPA 0x10000 (Zeroed guest IDT base), Limit 0x0FFF.

## 3. Extended Page Tables (EPT) Memory Architecture

- [VERIFIED-BY-CODE: kernel/core/hypervisor/src/ept.c:1-250]
- **Page Size**: 2 MB Large Pages mapped at EPT Level 2 (PD entries with bit 7 = 1 PS).
- **Mapping**:
  - GPA 0x00000000 to 0x80000000 (2048 MB) identity-mapped to HPA 0x00000000 to 0x80000000.
  - Attributes per entry: Read (bit 0 = 1), Write (bit 1 = 1), Execute (bit 2 = 1).
  - Memory Type: Write-Back (bits 5:3 = 6).
- **EPTP (EPT Pointer)**:
  - Formed as (pml4_hpa & ~0xFFFULL) | (3ULL << 3) | 6ULL (4-level walk, Write-Back).
  - Loaded into VMCS field 0x201A (VMCS_EPT_POINTER).

## 4. Hardware Transition & Register Assembly (vmx_entry.asm)

- [VERIFIED-BY-CODE: kernel/core/hypervisor/src/vmx_entry.asm:1-110]
- **Entry (vmx_launch_asm, vmx_resume_asm)**:
  - Receives vCPU *vcpu in RDI.
  - Pushes Host GPRs: push rbp, rbx, r12, r13, r14, r15.
  - Saves host RSP into vcpu->host_rsp.
  - Restores Guest GPRs from vcpu->guest_regs: rax, rbx, rcx, rdx, rsi, rdi, rbp, r8-r15.
  - Executes vmlaunch (initial entry) or vmresume (subsequent exits).
  - If launch fails (ZF=1 or CF=1): jumps to error handler, saves flags, restores host GPRs, returns failure.
- **Exit (vmx_vmexit_handler)**:
  - Hardware loads host RIP (vmx_vmexit_handler) and host RSP (vcpu->host_stack_top).
  - Pushes all Guest GPRs to vCPU structure.
  - Restores Host GPRs: pop r15, r14, r13, r12, rbx, rbp.
  - Reads VMCS_VM_EXIT_REASON and VMCS_VM_EXIT_INSTRUCTION_LEN.
  - Calls C dispatcher vmx_exit_handler(vcpu).

## 5. Comprehensive VM-Exit Dispatcher Table

| Exit Reason (Decimal) | Name | Handler in hypervisor.c | Emulation Behavior | RIP Advance | Unhandled / Fallback Behavior |
|:---|:---|:---|:---|:---|:---|
| **1** | EXTERNAL_INTR | lines 2069-2088 | Acknowledges host LAPIC; checks if timer/input requires servicing. | None (Guest interrupted) | Resumes guest or yields if quantum expired. |
| **2** | TRIPLE_FAULT | lines 2052-2063 | Logs fatal triple fault; sets shutdown_reason = VM_SHUTDOWN_GUEST_RESET; initiates 3s auto-reinit. | No | Halts guest execution. |
| **7** | INTERRUPT_WINDOW | lines 1819-1851 | Disables Interrupt-window exiting (bit 2) in VMCS; re-evaluates and injects highest pending interrupt. | No | Resumes guest. |
| **10** | CPUID | lines 1852-1912 | Calls virtual_platform_handle_cpuid(); returns synthetic Intel features (SSE, AVX, hypervisor present bit 31). | RIP += len | Resumes guest. |
| **12** | HLT | lines 1742-1818 | Checks if interrupts pending; if none, sets vcpu->halted = true and yields host CPU slice immediately. | RIP += len (upon wake) | Bypasses host spin loop. |
| **18** | VMCALL | lines 2045-2051 | Stubbed hypercall interface; returns -1 in RAX. | RIP += len | Resumes guest. |
| **30** | IO_INSTRUCTION | lines 1913-1969 | Decodes port, direction, size; calls virtual_platform_emulate_io(port, val, is_write). | RIP += len | Unhandled ports return 0xFF. |
| **31** | RDMSR | lines 1970-2008 | Emulates IA32_TIME_STAMP_COUNTER, APIC_BASE, MTRR, EFER, SYSENTER. | RIP += len | Unknown MSR returns 0. |
| **32** | WRMSR | lines 2009-2044 | Emulates TSC_DEADLINE, APIC_BASE, EFER, FS_BASE, GS_BASE. | RIP += len | Unknown MSR silently discarded. |
| **48** | EPT_VIOLATION | lines 2089-2110 | Inspects faulting GPA. If GPA belongs to IOAPIC (0xFEC00000) or LAPIC (0xFEE00000), calls MMIO handler. | None | Unmapped GPA halts VM with EPT fault. |
| **52** | PREEMPT_TIMER | lines 2064-2068 | Clears timer exit; yields vCPU quantum. | None | Resumes guest. |
| **55** | XSETBV | lines 2111-2135 | Guest writes XCR0. Validates feature mask (X87, SSE, AVX). Updates vcpu->xcr0. | RIP += len | Invalid mask injects #GP(0). |
| **All Others** | UNKNOWN | line 2205 | Logs unhandled reason and qualification; halts VM. | No | VM transitions to VM_STATE_STOPPED. |

## 6. Interrupt Injection Subsystem & Collision Matrix

### 6.1 Injection Priority Hierarchy
- [VERIFIED-BY-CODE: kernel/core/hypervisor/src/hypervisor.c:4015-4045]
- Before every vmresume, the hypervisor evaluates pending interrupts in strictly ordered sequence:
  1. **Level 1: LAPIC TSC-Deadline Timer** (atoms_hypervisor_try_inject_lapic_timer) -> Vector lapic->lvt_timer & 0xFF (typically Vector 0xEF).
  2. **Level 2: PS/2 Keyboard IRQ 1** (atoms_hypervisor_try_inject_kbd_irq) -> Vector via IOAPIC RTE[1] (typically Vector 0x31 or 0x21).
  3. **Level 3: VirtIO-Block IRQ 10** (atoms_hypervisor_try_inject_virtio_irq) -> Vector via IOAPIC RTE[10] (typically Vector 0x31 / 0x32).
  4. **Level 4: UART COM1 IRQ 4** (atoms_hypervisor_try_inject_uart_irq) -> Vector via IOAPIC RTE[4] (typically Vector 0x34).

### 6.2 Window Gating & Collision Prevention
- [VERIFIED-BY-CODE: kernel/core/hypervisor/src/hypervisor.c:725-750, 4016-4020]
- **Collision Guard**:
  Checks VMCS_VM_ENTRY_INTR_INFO_FIELD bit 31. If already set (0x80000000), avoids overwriting currently queued vector.
- **Interrupt Window Gating**:
  - If guest RFLAGS.IF == 0 (interrupts disabled) OR guest is in interruptibility state (STI blocking or MOV-SS blocking):
  - Hardware CANNOT accept vector injection.
  - Hypervisor enables Interrupt-window exiting (bit 2 in CPU-based controls).
  - The moment the guest executes STI and interrupt window opens, CPU causes VM-exit Reason 7 (INTERRUPT_WINDOW).
  - Exit handler disables bit 2 and cleanly delivers the pending vector.

## 7. Known Architectural Vulnerabilities & Risky Spots

1. **Host GDT Limit & TR Base Sharing** [VERIFIED-BY-CODE: kernel/core/hypervisor/src/hypervisor.c:3232, 3234]:
   - Guest GUEST_GDTR_BASE is initialized pointing to the HOST physical GDT.
   - If guest switches to user mode and inspects descriptor tables, it can view host kernel GDT entries until FreeBSD loads its own GDTR.
2. **Interrupt Vector Re-use Collisions**:
   - Both Keyboard (IRQ 1) and VirtIO (IRQ 10) frequently bind to Vector 0x31 under FreeBSD's interrupt routing. If both assert simultaneously, the queue prioritizes Keyboard; VirtIO must wait for the next exit cycle.
3. **MSR Emulation Gaps**:
   - MSRs for power management, thermal monitoring, and architectural perf counters return dummy 0. If FreeBSD kernel modules probe unsupported model-specific registers with strict fault handling, #GP is not injected; silent 0 is returned.

## 8. Top 10 Claims Least Sure About (Part 2)

1. Whether Intel Haswell / Raptor Lake errata affect Interrupt-window exiting during guest pause instruction loops.
2. The exact latency introduced when vmx_adjust_controls queries MSRs dynamically versus cached static bitmasks.
3. Whether GUEST_DR7 value 0x400 suppresses all hardware debug watchpoints set by guest GDB.
4. If guest FreeBSD userland ever executes RDTSC with CR4.TSD set, whether hypervisor properly catches it (RDTSC exiting is 0).
5. What happens if guest sets CR3 to a GPA that maps outside the 2048 MB EPT window.
6. Whether VMCS field VMCS_GUEST_INTERRUPTIBILITY_INFO can leave stale STI-blocking flags across an unexpected NMI.
7. Whether FreeBSD 14.1 ever attempts INVD (Invalidate Internal Caches), which is exit reason 13 (not explicitly handled in switch).
8. Behavior of vmx_vmwrite failure in release build where error checking is bypassed.
9. Whether VMX_EXIT_REASON_PREEMPT_TIMER can starve VirtIO disk queue processing during bursty disk transfers.
10. If FreeBSD modifies IA32_EFER.LMA (switches out of long mode), whether hypervisor will fatal crash on VM-entry consistency checks.
