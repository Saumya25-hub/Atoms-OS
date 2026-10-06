# PATCH REPORT — TASK 3 (PATCH TEAM)
## DETERMINISTIC VIRTIO PCI INTERRUPT PRESERVATION & INJECTION

**Document ID**: `PATCH_REPORT.md`  
**Protocol Phase**: TASK 3 — Patch Implementation  
**Inputs**: `FORENSIC_REPORT.md`, `PATCH_PLAN.md`  
**Author**: Antigravity Patch Team  
**Date**: October 6, 2026  

---

### 1. Files Changed
- `kernel/core/hypervisor/src/hypervisor.c`

*(No other files modified for the patch; conforms 100% to `PATCH_PLAN.md`).*

---

### 2. Functions Changed / Added
1. **Added Helper**:
   `static bool atoms_hypervisor_try_inject_virtio_irq(vCPU *vcpu)`
   - Implemented before `atoms_vmexit_dispatch()`.
   - Checks:
     - Hypervisor region and platform validity (`s_vmxon_region != NULL`, `vcpu->vm->platform != NULL`).
     - Pending flag: `vcpu->vm->virtio_irq_pending == true`.
     - Injection slot availability: `(vmx_vmread(VMCS_VM_ENTRY_INTR_INFO_FIELD) & 0x80000000U) == 0`.
     - Guest Interrupt Flag: `(vmx_vmread(VMCS_GUEST_RFLAGS) & (1ULL << 9)) != 0` (IF=1).
     - Interruptibility state: `(vmx_vmread(VMCS_GUEST_INTERRUPTIBILITY_INFO) & 3) == 0` (no STI / MOV-SS shadow).
     - IOAPIC RTE 11 validity: `vector >= 32 && !masked`.
   - Action upon match:
     - Programs `vmx_vmwrite(VMCS_VM_ENTRY_INTR_INFO_FIELD, 0x80000000U | (uint32_t)vector)`.
     - Clears `vcpu->vm->virtio_irq_pending = false` **only upon actual injection**.
     - Emits forensic telemetry.
   - Action if conditions not met:
     - Returns `false`.
     - **Preserves `virtio_irq_pending = true`** (never discards pending interrupt prematurely).

2. **Modified `atoms_vmexit_dispatch()` — `case VMX_EXIT_REASON_HLT:`**:
   - Replaced inline destructive check (which previously cleared `virtio_irq_pending = false` when RTE 11 was masked/uninitialized) with a call to `atoms_hypervisor_try_inject_virtio_irq(vcpu)`.
   - If VirtIO IRQ was not injected, fallback to virtual LAPIC timer vector 0x20 is preserved.

3. **Modified `atoms_vmexit_dispatch()` — `case VMX_EXIT_REASON_IO_INSTRUCTION:`**:
   - Added immediate injection attempt via `atoms_hypervisor_try_inject_virtio_irq(vcpu)` on VM-entry resumption from I/O exits (such as `QUEUE_NOTIFY`).

4. **Modified `atoms_vmexit_dispatch()` — `case VMX_EXIT_REASON_EPT_VIOLATION:`**:
   - Added injection attempt when IOAPIC MMIO space (`0xFEC00000`–`0xFEC01000`) is modified/unmasked by the guest.

5. **Modified Generic VM-Entry Resumption Paths**:
   - In `atoms_vcpu_run()` (lines ~2655) and `atoms_hypervisor_runtime_step()` (lines ~2792), check `atoms_hypervisor_try_inject_virtio_irq(vcpu)` whenever `vcpu->vm->virtio_irq_pending` is true on `VMEXIT_HANDLED_AND_RESUME`.

---

### 3. Lines Changed
- Lines added / modified in `kernel/core/hypervisor/src/hypervisor.c`:
  - Lines 575–654: Added `atoms_hypervisor_try_inject_virtio_irq()`.
  - Lines 710–725: Refactored `case VMX_EXIT_REASON_HLT:`.
  - Lines 825–831: Hooked `case VMX_EXIT_REASON_IO_INSTRUCTION:`.
  - Lines 900–905: Hooked `case VMX_EXIT_REASON_EPT_VIOLATION:`.
  - Lines 2655–2660: Hooked `atoms_vcpu_run()`.
  - Lines 2792–2797: Hooked `atoms_hypervisor_runtime_step()`.

---

### 4. Build & Safety Verification
- Compiles with 0 warnings / 0 errors.
- Unrelated subsystems touched: **0**.
