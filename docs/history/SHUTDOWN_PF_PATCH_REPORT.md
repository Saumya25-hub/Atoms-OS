# ATOMS OS — PATCH REPORT
## FIX: Shutdown & Reboot Page Fault (#PF) Mitigation

**Date**: 2026-09-15  
**Author**: ATOMS Engineering Team  
**Status**: APPLIED (TASK 3)  

---

### 1. Summary of Changes

To eliminate `#PF` (Vector 14) exceptions caused by reading UEFI Configuration Tables and ACPI physical memory (`0x5F5A5E00`) from an unmapped user-task CR3 during shutdown/reboot:
1. Restored the root kernel page table (`g_kernel_pml4`) with 0..512 GB identity mapping upon entering `atoms_power_shutdown()`, `atoms_power_reboot()`, `system_shutdown()`, and `system_reboot()`.
2. Disabled CPU interrupts (`cli`) immediately at the start of `system_shutdown()` and `system_reboot()` to prevent preemption or context switching during powerdown.
3. Added UEFI ACPI GUID validation in `find_acpi_rsdp()` to ensure only genuine ACPI 1.0/2.0 table pointers are inspected.
4. Added AML length bounds checking in `parse_s5_from_dsdt()`.

---

### 2. Files & Functions Changed

| File | Functions Changed | Description |
|---|---|---|
| [`kernel/core/power/system_power.c`](file:///d:/Signatures_OS/kernel/core/power/system_power.c) | `atoms_power_shutdown` | Switch CR3 to `vmm_get_kernel_pml4()`. |
| [`kernel/core/power/system_power.c`](file:///d:/Signatures_OS/kernel/core/power/system_power.c) | `atoms_power_reboot` | Switch CR3 to `vmm_get_kernel_pml4()`. |
| [`kernel/core/power/system_power.c`](file:///d:/Signatures_OS/kernel/core/power/system_power.c) | `system_shutdown` | `cli` + switch CR3 to `vmm_get_kernel_pml4()`. |
| [`kernel/core/power/system_power.c`](file:///d:/Signatures_OS/kernel/core/power/system_power.c) | `system_reboot` | `cli` + switch CR3 to `vmm_get_kernel_pml4()`. |
| [`kernel/core/power/system_power.c`](file:///d:/Signatures_OS/kernel/core/power/system_power.c) | `find_acpi_rsdp` | Verify `EFI_ACPI_20_TABLE_GUID` & `EFI_ACPI_TABLE_GUID`. |
| [`kernel/core/power/system_power.c`](file:///d:/Signatures_OS/kernel/core/power/system_power.c) | `parse_s5_from_dsdt` | Added length bounds checks against `aml_len`. |

---

### 3. Verification

- All modified files conform to freestanding C99 kernel standards.
- Zero breaking API modifications or side effects to non-power subsystems.
