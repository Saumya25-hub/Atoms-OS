# ATOMS OS — ARCHITECTURE PATCH PLAN (TASK 2)
### TARGET: ELIMINATION OF VMX EXTERNAL INTERRUPT STORM (VM EXIT REASON 1) AT GUEST RIP 0xFFFFFFFF80FD1D78

**Protocol Phase**: TASK 2 — ARCHITECT TEAM (RULE 0 Mandatory Phase Isolation)  
**Target Hardware**: Physical Intel Core i3-14100F (Raptor Lake LGA1700) / ASUS PRIME B760M-K  
**Failing Guest Address**: `0xFFFFFFFF80FD1D78` (`FreeBSD ucode_load_bsp + 0x18`)  
**Target Subsystem**: Intel VMX Exit Dispatcher (`kernel/core/hypervisor/src/hypervisor.c`)  
**Date**: September 30, 2026  
**Status**: ARCHITECTURAL PROPOSAL FOR OPERATOR APPROVAL (STRICTLY READ-ONLY — NO CODE MODIFIED)  

---

## 1. Executive Summary & Root Cause Analysis

### 1.1 The Observed Failure
On physical Intel hardware, the FreeBSD guest reaches:
```text
Guest RIP    : 0xFFFFFFFF80FD1D78 (FreeBSD ucode_load_bsp + 0x18)
Instruction  : 89 4D C8 -> movl %ecx, -0x38(%rbp)
LastExit     : 0x0001 (VMX_EXIT_REASON_EXTERNAL_INTR)
Disposition  : VMEXIT_HANDLED_AND_RESUME
Total Exits  : 3,000,000,000+ (Storming at ~5.8M exits/sec)
Guest Prog.  : 0 bytes advanced (Guest RIP completely frozen)
```

### 1.2 Architectural Root Cause
In Task 3 (TRAP 30 fix), `VMCS_PIN_BASED_VM_EXEC_CONTROL` Bit 0 (`External-interrupt exiting`) was enabled (`1U << 0`) to prevent physical interrupts from penetrating into uninitialized guest IDT descriptors (`Xrsvd`).

However, the VMX exit dispatcher in `kernel/core/hypervisor/src/hypervisor.c` lines 780–784 was left as a no-op stub:
```c
case VMX_EXIT_REASON_PREEMPT_TIMER:
case VMX_EXIT_REASON_EXTERNAL_INTR:
    vcpu->last_exit.handled = true;
    vcpu->last_exit.disposition = VMEXIT_HANDLED_AND_RESUME;
    return true;
```

Per **Intel SDM Vol 3C §27.5.3**, VM-exits clear `RFLAGS.IF` to 0. The host executes in VMX root mode with interrupts disabled.
Per **Intel SDM Vol 3C §24.7.1**, because `Acknowledge interrupt on exit` (VM-exit control Bit 15) is 0, the CPU does **not** acknowledge the interrupt controller on VM-exit. The physical interrupt remains asserted and pending.
Per **Intel SDM Vol 3C §26.5.1**:
> *"If the 'external-interrupt exiting' VM-execution control is 1 and an external interrupt is pending (for example, signaled on an INTR pin or by local APIC), a VM exit occurs immediately after VM entry before any instruction in the guest is executed. The exit reason is 1 (external interrupt)."*

Because ATOMS OS marks the exit as handled and immediately calls `vmresume` without ever unmasking host interrupts, executing an INTA cycle, or issuing an EOI to the interrupt controller:
1. The hardware interrupt remains continuously asserted on the CPU INTR line.
2. `vmresume` re-enters VMX non-root mode.
3. The CPU detects the pending external interrupt before executing even a single instruction.
4. The CPU immediately exits back to the host with Exit Reason 1.
5. Guest RIP never moves from `0xFFFFFFFF80FD1D78`.
6. This cycle repeats billions of times, locking the system in an external interrupt storm.

---

## 2. Forensic Proof of Pending Interrupt Source

Based on direct examination of the ATOMS OS kernel codebase and boot configuration:

1. **8259 PIC & 8254 PIT (PROVEN)**:
   - In [`kernel/kernel.c:452`](file:///d:/Signatures_OS/kernel/kernel.c#L452), `pic_init()` configures the Master 8259 PIC (port `0x20`) and Slave PIC (port `0xA0`).
   - In [`kernel/kernel.c:460`](file:///d:/Signatures_OS/kernel/kernel.c#L460), `irq_register_handler(0, timer_irq_handler)` binds IRQ0 to vector 32 (`0x20`).
   - In [`kernel/kernel.c:468`](file:///d:/Signatures_OS/kernel/kernel.c#L468), `pic_clear_mask(0)` unmasks IRQ0 on the Master PIC.
   - In [`drivers/timer/pit/pit.c:27-34`](file:///d:/Signatures_OS/drivers/timer/pit/pit.c#L27-L34), `pit_timer_driver` programs 8254 PIT Channel 0 (ports `0x40`/`0x43`) in Square Wave Mode (0x36) to pulse at 1000 Hz.
   - The 8254 PIT is physically wired to IRQ0 of the Master PIC. When IRQ0 fires, the 8259 PIC raises the CPU's INTR line.
   - The PIC continues to hold INTR high until it receives an INTA acknowledge cycle and an End-of-Interrupt (EOI = `0x20`) command sent to `PIC1_CMD` (port `0x20`).
   - Because the hypervisor never issued EOI, IRQ0 is permanently pending.

2. **Local APIC (Eliminated as Host Source)**:
   - On the BSP (Bootstrap Processor), ATOMS OS runs strictly on the legacy 8259 PIC.
   - [`arch/x86_64/smp/smp.c:646`](file:///d:/Signatures_OS/arch/x86_64/smp/smp.c#L646) (`atoms_lapic_enable_foundation()`) is **never called on the BSP**; it is only invoked for secondary APs.
   - LAPIC timer is not active on the BSP host.

3. **IOAPIC (Eliminated as Host Source)**:
   - Host ATOMS OS does not route host interrupts through physical IOAPIC.
   - The IOAPIC logic in [`kernel/core/hypervisor/src/virtual_platform.c`](file:///d:/Signatures_OS/kernel/core/hypervisor/src/virtual_platform.c) is strictly an emulation layer for the FreeBSD guest at GPA `0xFEC00000`.

4. **PCI Devices (RTL8125, USB xHCI)**:
   - Polled in software (`net_poll()` in [`hypervisor_dashboard.c:2479`](file:///d:/Signatures_OS/kernel/debug/hypervisor_dashboard/hypervisor_dashboard.c#L2479)); neither driver has an unmasked legacy PIC IRQ in `irq_register_handler`.

**Conclusive Determination**:
The physical pending interrupt source is **IRQ0 (8254 PIT / Master 8259 PIC Timer)** mapped to host IDT vector 32 (`0x20`).

---

## 3. Host VMX Root Mode Execution Flow & Mechanism

```mermaid
sequenceDiagram
    autonumber
    participant Guest as FreeBSD Guest (Non-Root)
    participant CPU as Intel Core i3 (Silicon)
    participant VMExit as vmx_vmexit_handler (Host ASM)
    participant Step as atoms_hypervisor_runtime_step (Host C)
    participant Disp as atoms_vmexit_dispatch (Host C)
    participant IDT as Host IDT & isr32 / irq_dispatch
    participant PIC as Master 8259 PIC (Port 0x20)

    Guest->>CPU: Executing 0xFFFFFFFF80FD1D78 (ucode_load_bsp)
    PIC->>CPU: Physical PIT IRQ0 Asserted (INTR Pin HIGH)
    CPU->>CPU: Detects Pin-based Bit 0=1 (External Interrupt Exiting)
    CPU->>CPU: Clears RFLAGS.IF=0, Saves Guest State to VMCS
    CPU->>VMExit: Transfers control to VMCS_HOST_RIP (Host RSP)
    VMExit->>Step: Saves Guest GPRs, restores Host Stack, returns 1
    Step->>Disp: calls atoms_vmexit_dispatch(vcpu) [Exit Reason 1]
    
    rect rgb(20, 60, 30)
    Note over Disp,IDT: Proposed Minimal Architectural Fix
    Disp->>CPU: Executes "sti; nop; cli" (Opens Host Interrupt Window)
    CPU->>PIC: Issues Hardware INTA Cycle -> Receives Vector 0x20 (32)
    CPU->>IDT: Traps to isr32 -> isr_common_handler -> irq_dispatch(32)
    IDT->>IDT: Calls timer_irq_handler() -> increments g_timer_ticks
    IDT->>PIC: Calls pic_send_eoi(0) -> Out 0x20 to Port 0x20
    PIC->>CPU: De-asserts INTR line (Interrupt Cleared)
    IDT->>Disp: iretq returns to cli (RFLAGS.IF restored to 0)
    end

    Disp->>Step: Returns VMEXIT_HANDLED_AND_RESUME
    Step->>CPU: Executes vmresume (vmx_run_vcpu_raw)
    CPU->>Guest: Enters Non-Root: INTR line is LOW -> FreeBSD executes!
    Guest->>Guest: Completes movl %ecx, -0x38(%rbp) and advances
```

### Detailed Trace & Mechanism Analysis:
1. When `Acknowledge interrupt on exit` is 0, the interrupt controller is not acknowledged during VM-exit.
2. The host kernel environment in VMX root mode is 100% valid:
   - Host RSP has been switched back to the host kernel stack (`g_saved_host_rsp`).
   - Host IDTR points to [`arch/x86_64/interrupt/idt.c`](file:///d:/Signatures_OS/arch/x86_64/interrupt/idt.c) (`&idt[0]`).
   - Host GDT and selectors are valid (`CS=0x08`, `SS=0x10`, `DS=0x10`).
   - Host CR3 is kernel PML4.
3. By opening a momentary interrupt window via inline assembly:
   ```c
   __asm__ volatile (
       "sti\n\t"
       "nop\n\t"
       "cli\n\t"
       : : : "memory"
   );
   ```
   - `sti` enables interrupts starting after the next instruction (`nop`).
   - At the `nop` boundary, the CPU recognizes the pending physical interrupt on the INTR pin.
   - The CPU executes an INTA cycle with the 8259 PIC, receiving vector 32 (`0x20`).
   - The CPU pushes the interrupt frame onto the host kernel stack and branches through host IDT gate 32 to `isr32` in [`arch/x86_64/interrupt/isr_stubs.asm`](file:///d:/Signatures_OS/arch/x86_64/interrupt/isr_stubs.asm).
   - `isr_stubs.asm` saves all host GPRs, calls `isr_common_handler()`, which calls [`kernel/core/interrupt/src/irq.c:irq_dispatch()`](file:///d:/Signatures_OS/kernel/core/interrupt/src/irq.c#L30).
   - `irq_dispatch()` invokes the registered handler: `timer_irq_handler()` in `kernel/kernel.c:147`, which increments `g_timer_ticks` and returns 0.
   - `irq_dispatch()` invokes [`drivers/interrupt/pic/pic.c:pic_send_eoi(0)`](file:///d:/Signatures_OS/drivers/interrupt/pic/pic.c#L63), outputting `0x20` to `PIC1_CMD` (port `0x20`).
   - The 8259 PIC clears the in-service state and releases the INTR line.
   - `isr_stubs.asm` executes `iretq`, returning to the instruction following `nop`, which is `cli`.
   - `cli` immediately clears `RFLAGS.IF = 0`.
4. `atoms_vmexit_dispatch()` returns `true` with `disposition = VMEXIT_HANDLED_AND_RESUME`.
5. `atoms_hypervisor_runtime_step()` executes `vmresume`.
6. Because the pending interrupt was serviced and cleared at the PIC, the CPU enters VMX non-root mode cleanly.
7. FreeBSD executes instruction `movl %ecx, -0x38(%rbp)` at `0xFFFFFFFF80FD1D78` and advances to subsequent instructions.

---

## 4. Evaluation of Alternative Approaches

| Approach | Mechanism | Pros | Cons / Verdict |
| :--- | :--- | :--- | :--- |
| **Approach 1 (Recommended): Root-Mode Host Interrupt Window (`sti; nop; cli`)** | Leave `VM_EXIT_CONTROLS` Bit 15 = 0. Briefly enable host IF in `atoms_vmexit_dispatch()`. | • Zero VMCS control changes.<br>• Reuses existing certified host IDT, ISR assembly, and PIC EOI pipeline.<br>• Zero risk of VM-entry failure or VMCS consistency check failure.<br>• Minimal patch (6 lines of code in 1 file). | **RECOMMENDED & CHOSEN**. Standard VMX practice (identical to early Linux KVM). |
| **Approach 2: Acknowledge Interrupt On Exit (Exit Control Bit 15 = 1)** | Set VM-exit control Bit 15. Processor reads vector into `VMCS_VM_EXIT_INTR_INFO`. | Hardware performs INTA during VM-exit. | **REJECTED**: Per Intel SDM §27.2.1, hardware does *not* invoke host IDT. Hypervisor must manually construct `registers_t`, manually call ISRs, and manually send PIC EOI. High risk of software vector dispatch mismatch. |
| **Approach 3: Direct Manual PIC EOI in Dispatcher without Host Interrupt Window** | Call `pic_send_eoi(0)` directly in `case VMX_EXIT_REASON_EXTERNAL_INTR:`. | Fast. | **REJECTED**: If `Acknowledge interrupt on exit` is 0, the PIC has not received an INTA cycle, so issuing EOI without INTA corrupts 8259 internal state. Furthermore, host timer ticks would not increment. |
| **Approach 4: Mask IRQ0 in PIC before VM Entry** | Call `pic_set_mask(0)` before `vmresume` and unmask after. | Stops interrupt. | **REJECTED**: Loses all host timer ticks, stalls system time, and creates port I/O overhead on every VM exit. |

---

## 5. Scope of Changes (Approved File Only)

Per RULE 0: Only **ONE** file is approved for modification in Task 3:

```text
[APPROVED FILE]
d:/Signatures_OS/kernel/core/hypervisor/src/hypervisor.c
```

**STRICT PROHIBITION**:
- NO modifications to Realtek RTL8125 driver or VirtIO / vtnet.
- NO modifications to DHCP, DNS, TCP, or HTTPS.
- NO modifications to FreeBSD kernel or ELF payload.
- NO modifications to CPUID, XSETBV, ACPI, DSDT, PCI, graphics, or HLT.
- NO modifications to `kernel.c`, `smp.c`, `irq.c`, `pic.c`, or `vmx_entry.asm`.

---

## 6. Detailed Patch Specification (For Task 3)

### File: `kernel/core/hypervisor/src/hypervisor.c`
- **Function**: `atoms_vmexit_dispatch(vCPU *vcpu)`
- **Lines**: 779–784
- **Current Code**:
  ```c
              case VMX_EXIT_REASON_PREEMPT_TIMER:
              case VMX_EXIT_REASON_EXTERNAL_INTR:
                  vcpu->last_exit.handled = true;
                  vcpu->last_exit.disposition = VMEXIT_HANDLED_AND_RESUME;
                  return true;
  ```
- **Target Replacement Code**:
  ```c
              case VMX_EXIT_REASON_PREEMPT_TIMER:
                  vcpu->last_exit.handled = true;
                  vcpu->last_exit.disposition = VMEXIT_HANDLED_AND_RESUME;
                  return true;

              case VMX_EXIT_REASON_EXTERNAL_INTR:
                  /*
                   * Intel VMX Architecture:
                   * Pin-based Bit 0 (External-interrupt exiting) intercepted a physical
                   * host interrupt (8254 PIT / 8259 PIC IRQ0) to prevent guest IDT corruption.
                   * Because 'Acknowledge interrupt on exit' is 0, the interrupt remains
                   * pending at the interrupt controller and host RFLAGS.IF was cleared to 0.
                   *
                   * Briefly open an interrupt window in VMX root mode. The CPU delivers the
                   * pending interrupt through the host IDT (isr32 -> irq_dispatch), which
                   * updates host timer ticks and issues pic_send_eoi(0). 'cli' then re-masks
                   * interrupts so vmresume can safely resume the FreeBSD guest without an
                   * immediate re-exit storm.
                   */
                  __asm__ volatile (
                      "sti\n\t"
                      "nop\n\t"
                      "cli\n\t"
                      : : : "memory"
                  );
                  vcpu->last_exit.handled = true;
                  vcpu->last_exit.disposition = VMEXIT_HANDLED_AND_RESUME;
                  return true;
  ```

---

## 7. Required Safety Checks & Invariant Audits

1. **Host Stack Space**:
   - `atoms_vmexit_dispatch()` is executed in C on the host kernel stack (`g_saved_host_rsp`).
   - The interrupt frame pushed by CPU hardware and `isr_common_stub` consumes ~184 bytes.
   - The host stack has dozens of kilobytes of available headroom. No risk of stack overflow.

2. **Reentrancy & Interrupt Handlers**:
   - The registered handler for IRQ0 during hypervisor execution is `timer_irq_handler()` in [`kernel/kernel.c:147`](file:///d:/Signatures_OS/kernel/kernel.c#L147):
     ```c
     static uint64_t timer_irq_handler(registers_t *regs) {
         (void)regs;
         g_timer_ticks++;
         return 0;
     }
     ```
   - It performs zero context switching, calls no scheduler routines, touches no hypervisor structures, and returns 0.
   - No reentrancy hazard exists.

3. **PIC EOI Assurance**:
   - [`kernel/core/interrupt/src/irq.c:43`](file:///d:/Signatures_OS/kernel/core/interrupt/src/irq.c#L43) unconditionally issues `pic_send_eoi(irq)` for any hardware vector between 32 and 47.
   - Master PIC IRQ0 is guaranteed to be EOI'd on every handled tick.

4. **Guaranteed Host IF = 0 on `vmresume`**:
   - The `cli` instruction in `"sti\n\tnop\n\tcli\n\t"` executes immediately after the `iretq` completes.
   - This ensures host `RFLAGS.IF` is strictly 0 prior to `vmresume`.

5. **No Trap 30 Regression**:
   - `VMCS_PIN_BASED_VM_EXEC_CONTROL` Bit 0 remains 1.
   - Physical hardware interrupts will NEVER penetrate directly into FreeBSD's guest IDT. Trap 30 cannot reoccur.

---

## 8. Expected Telemetry & Verification Criteria

Following the application of this patch in Task 3:

1. **Exit Storm Elimination**:
   - VM-exit rate will drop from millions per second down to the normal guest execution profile (~1,000 timer exits/sec plus occasional I/O exits).
2. **Guest Progression**:
   - Guest RIP will advance past `0xFFFFFFFF80FD1D78`.
   - FreeBSD `ucode_load_bsp()` will return cleanly.
   - FreeBSD execution will advance through `hammer_time` into `mi_startup()`.
3. **Serial Telemetry**:
   - `LastExit` will transition between various exit reasons as FreeBSD executes real OS bootstrap code instead of being pinned to `0x0001`.
   - VMCS exit counters will reflect genuine instruction execution.

---

## 9. Rollback Plan

If unexpected behavior occurs:
```bash
git checkout kernel/core/hypervisor/src/hypervisor.c
```
Reverts `hypervisor.c` back to the pre-patch baseline.
