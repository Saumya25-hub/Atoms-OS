# ATOMS OS — ARCHITECTURE PATCH PLAN
## FIX: Shutdown & Reboot Kernel Address Space & Interrupt Quiescing

**Target Issue**: `#PF` (Vector 14) during bare-metal hardware shutdown at `CR2: 0x5F5A5E00`.  
**Input Document**: `docs/history/SHUTDOWN_PF_FORENSIC_REPORT.md`  
**Status**: PROPOSED (TASK 2 - NO CODE)  

---

### 1. Scope & Objective

Ensure the power transition subsystem (`system_power.c`) operates strictly under the root kernel page table (`g_kernel_pml4`) with interrupts disabled, allowing safe, fault-free dereferencing of UEFI runtime tables, ACPI tables (`RSDP`, `XSDT`, `FADT`, `DSDT`), and hardware I/O ports during system shutdown and reboot.

---

### 2. Files to Modify

| File | Subsystem | Modifications Planned |
|---|---|---|
| [`kernel/core/power/system_power.c`](file:///d:/Signatures_OS/kernel/core/power/system_power.c) | Power Management | 1. Restore `g_kernel_pml4` in `atoms_power_shutdown()`, `atoms_power_reboot()`, `system_shutdown()`, and `system_reboot()`.<br>2. Disable interrupts (`cli`) before starting ACPI walk.<br>3. Add AML length bounds validation in `parse_s5_from_dsdt()`.<br>4. Validate UEFI Configuration Table ACPI GUIDs. |

---

### 3. Rationale & Expected Results

- **Why**: Process address spaces isolate `0x40000000..0x6FFFFFFF` for user programs. UEFI firmware maps ACPI tables directly into physical memory at addresses like `0x5F5A5E00`. Restoring `g_kernel_pml4` gives the kernel access to its 0..512 GB identity map.
- **Expected Result**:
  - Clean shutdown sequence with 0 `#PF` exceptions.
  - Correct ACPI `\_S5_` sleep state trigger or clean Intel PCH PMBASE shutdown.
  - Clean safe halt without triggering the ABDE IDT failure screen.

---

### 4. Risk Assessment & Rollback Plan

- **Risk**: None. Normal OS execution is unaffected because power management functions are only called at the very end of system lifecycle.
- **Rollback Plan**: Revert `system_power.c` changes via `git checkout kernel/core/power/system_power.c`.
