# PATCH PLAN: Boundary B13 — EPT MMIO Instruction Register Decoding for IOAPIC & VirtIO

**Target Subsystem**: Hypervisor Core VM-Exit Intercept Engine (`kernel/core/hypervisor/src/hypervisor.c`)  
**Input Document**: `FORENSIC_REPORT.md` (Forensic Investigation of Boundary B13)  
**Failure Boundary**: Boundary B13 (VirtIO-BLK Read Completion IRQ 11 / LAPIC Vector 49 Injection)  
**Hardware Target**: Physical ASUS PRIME B760M-K / Intel Core i3-14100F (Haswell/Raptor Lake x86_64)  
**Protocol Phase**: Phase 2 (Architecture Plan — NO PRODUCTION CODE MODIFIED)  

---

## 1. What to Modify

### Primary File: `kernel/core/hypervisor/src/hypervisor.c`
Within the `VMX_EXIT_REASON_EPT_VIOLATION` handler block (lines 888–915):

1. **Replace the hardcoded `RAX` data register assumption**:
   - Currently, lines 893–896 read write data solely from `vcpu->guest_regs.rax` and write read results solely to `vcpu->guest_regs.rax`.
   - Implement an architectural MMIO instruction decoder function (`atoms_hypervisor_emulate_mmio_access`) that inspects the guest instruction opcode and ModR/M byte at `vcpu->guest_regs.rip`.

2. **Supported Instruction Forms for MMIO Emulation**:
   - **Opcode `0x89` (`MOV r/m32, r32`)**: Extract source register from ModR/M `reg` field (with REX.R prefix extension). Fetch value from the corresponding general-purpose register in `vcpu->guest_regs` (e.g., `%edx` for `IOREGSEL`).
   - **Opcode `0x8B` (`MOV r32, r/m32`)**: Extract destination register from ModR/M `reg` field (with REX.R prefix extension). Store the MMIO read return value into the corresponding general-purpose register in `vcpu->guest_regs` (e.g., `%r14d` for `ioapic_create` version read).
   - **Opcode `0xC7` (`MOV r/m32, imm32`)**: Extract the 32-bit immediate constant from the instruction stream (e.g., `$1` or `$0` for `IOREGSEL`). Pass the immediate value to `virtual_platform_handle_mmio`.

3. **Guest Virtual Address to GPA Translation for RIP**:
   - Implement a lightweight 4-level page table walk starting from guest CR3 (`vmx_vmread(VMCS_GUEST_CR3)` or `vcpu->cr3`) through guest RAM backing (`guest_mem->hva_backing`) to locate the instruction bytes for `vcpu->guest_regs.rip`.
   - Include direct KVA fallback: if `rip >= 0xFFFFFFFF80000000ULL`, direct translation via `rip - 0xFFFFFFFF80000000ULL` into `hva_backing`.

4. **Forensic Telemetry on MMIO Access (Flushed to UDP 9999)**:
   - For all accesses to `0xFEC00000`–`0xFEC00FFF` (IOAPIC), log the decoded operation:
     - Access type (`RD` / `WR`)
     - GPA offset (`0x00` IOREGSEL, `0x10` IOWIN)
     - Register involved (e.g., `RDX`, `RAX`, `R14`)
     - Value transferred
     - Resulting `IOREGSEL` index or `redirection_table` entry update.

---

## 2. Why

1. **FreeBSD Kernel Calling Convention & Code Generation**:
   FreeBSD's `sys/x86/x86/io_apic.c` compiles `ioapic_program_intpin` into:
   ```asm
   ffffffff810cd137: movl %edx, (%rcx)     # Write 0x27 (RTE 11 High) to IOREGSEL from %edx
   ffffffff810cd139: movl %eax, 0x10(%rcx) # Write High word to IOWIN from %eax
   ffffffff810cd156: movl %edx, (%rcx)     # Write 0x26 (RTE 11 Low) to IOREGSEL from %edx
   ffffffff810cd158: movl %eax, 0x10(%rcx) # Write Low word (Vector 49) to IOWIN from %eax
   ```
   The hypervisor's existing code hardcodes `uint64_t mmio_val = vcpu->guest_regs.rax;`. Because `%edx` holds the index and `%eax` holds the data, the hypervisor writes `%eax` into `IOREGSEL` instead of `%edx`.
2. **IOAPIC Index Desynchronization**:
   When writing the low word of RTE 11, `%eax` contains the vector payload (`0x0000a031`). Writing this to `IOREGSEL` sets `ioapic->ioregsel = 0xa031`. When the subsequent write to `IOWIN` occurs, `virtual_platform.c` checks `if (reg >= 0x10 && reg <= 0x3F)`. Because `0xa031 > 0x3F`, the write is silently discarded.
3. **Stale Hardware Version / ID**:
   During `ioapic_create`, FreeBSD reads `IOWIN` into `%r14d` (`movl 0x10(%rbx), %r14d`). The hypervisor wrote the read result to `rax`, leaving `%r14d` uninitialized, causing the observed warnings:
   `ioapic0: MADT APIC ID 1 != hw id 0` and `ioapic0: ver 0xa0 maxredir 0x0c`.
4. **Resolution**:
   Decoding the actual register operands in the instruction guarantees that `IOREGSEL` receives `0x27` and `0x26`, `IOWIN` receives vector 49 (`0x31`), and `redirection_table[11]` is correctly populated.

---

## 3. Expected Result

1. FreeBSD `ioapic0` initialization will correctly read hardware ID `1` and maxredir `23` (`0x17`):
   - Warning `ioapic0: MADT APIC ID 1 != hw id 0` will disappear.
   - String will show `ioapic0 <Version 0.20> irqs 0-23`.
2. When FreeBSD executes `ioapic0: routing intpin 11 (ISA IRQ 11) to lapic 0 vector 49`:
   - `IOREGSEL` will be set to `0x27`, then `0x26`.
   - `redirection_table[11]` will be updated from `0x0000000000000000` to `0x000000000000A031` (Vector 49, unmasked, level trigger, active low).
3. When VirtIO-BLK completes the sector read request during `mountroot`:
   - `virtio_irq_pending` is asserted `true`.
   - The injection gate in `hypervisor.c` reads `rte = plat->ioapic.redirection_table[11]`.
   - `vector` evaluates to `49` (`0x31 >= 32`) and `masked` is `false`.
   - Interrupt vector 49 is injected into `VMCS_VM_ENTRY_INTR_INFO_FIELD`.
4. FreeBSD services interrupt 49, completes the UFS superblock read, and successfully mounts the root filesystem from `/dev/vtbd0`.

---

## 4. Risk Analysis & Mitigation

* **Scope Isolation**: Modifications are strictly confined to the `VMX_EXIT_REASON_EPT_VIOLATION` dispatch logic in `hypervisor.c`.
* **Zero Impact on RAM Accesses**: Standard guest RAM accesses do not trigger EPT violations because RAM is mapped identity in EPT. Only unmapped MMIO ranges (`0xFEC00000`, `0xFEE00000`, `0xFEB00000`) invoke this path.
* **Instruction Length Safety**: `vcpu->last_exit.instruction_length` is confirmed valid and non-zero across all observed EPT violations on this CPU.
* **Risk Level**: Low to moderate. Standard hypervisor instruction emulation practice.

---

## 5. Rollback Plan

If regression occurs, revert the EPT violation MMIO dispatch block in `kernel/core/hypervisor/src/hypervisor.c` back to the baseline `rax`-direct assignment.
