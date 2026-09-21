# ATOMS OS — FORENSIC INVESTIGATION REPORT
## INCIDENT: Shutdown Trigger #PF (Page Fault) Exception (CR2: 0x5F5A5E00)

**Date**: 2026-09-15  
**Target Architecture**: Haswell / 14th Gen UEFI Bare-Metal (x86_64)  
**Status**: INVESTIGATION COMPLETE (TASK 1)  

---

### 1. Root Cause Analysis

When a user initiates system shutdown from the ATOMS Desktop (Start Menu / Horse Engine / Rook Power transition):
1. The execution path flows from `horse_shutdown()` ➔ `atoms_power_shutdown()` ➔ `rook_shutdown_spin()` ➔ `system_shutdown()`.
2. At the time of shutdown initiation, the active CPU paging directory (hardware register `CR3`) is the **Task/Desktop Process Address Space** created via `vmm_create_address_space()`.
3. In `vmm_create_address_space()`, the process page table `user_pd1` maps the range `0x40000000..0x80000000` (1 GB to 2 GB). To preserve user process isolation, entries `0..383` (`0x40000000..0x6FFFFFFF`) are strictly zeroed out (`user_pd1[i] = 0`) to allow pure on-demand 4KB user pages.
4. When `system_shutdown()` executes, it attempts to read UEFI Configuration Tables and parse ACPI tables via `find_acpi_rsdp()`, `find_acpi_fadt()`, and `parse_s5_from_dsdt()`.
5. On modern UEFI bare-metal hardware (such as ASUS PRIME B760M-K / H81), UEFI firmware allocates the ACPI RSDP, XSDT, FADT, and DSDT tables in ACPI Reclaim / Runtime physical memory in the 1.4 GB to 1.8 GB range (specifically at fault address `CR2 = 0x000000005F5A5E00`, which is ~1.489 GB).
6. Because `system_shutdown()` did NOT restore the root kernel page table (`g_kernel_pml4`), the MMU walked `user_pd1[250]`, encountered a `PRESENT = 0` bit, and immediately threw a Ring 0 Page Fault Exception (`#PF`, Vector 14).
7. The IDT exception handler intercepted the Ring 0 `#PF` and displayed the ABDE diagnostic panic screen: `IDT FAILED (SAFE HALT) #PF RIP:000000000002983CD CR2:000000005F5A5E00 ERR:00000000`.

---

### 2. Forensic Evidence

1. **Fault Address Breakdown**:
   - `CR2 = 0x000000005F5A5E00`
   - Linear Address: `1,599,725,056 bytes` (`~1.489 GB`).
   - Page Directory Level 2 Index in `user_pd1`:
     $$\text{Index} = \frac{0x5F5A5E00 - 0x40000000}{0x200000} = \frac{0x1F5A5E00}{0x200000} = 250$$
   - In `vmm_create_address_space()`, index 250 falls in the range `0..383` which is explicitly set to `0` (Unmapped).

2. **Kernel Identity Mapping vs User Address Space**:
   - In `g_kernel_pml4` (`vmm.c:174-186`), the entire physical range 0..512 GB is 100% identity mapped (0..4 GB via 2 MB huge pages in `pd0..pd3`, and 4 GB..512 GB via 1 GB huge pages in `pdp[4..511]`).
   - In any user task PML4, memory between `0x40000000` and `0x6FFFFFFF` is NOT identity mapped.

3. **Shutdown Context**:
   - Neither `atoms_power_shutdown()`, `rook_shutdown_spin()`, nor `system_shutdown()` switched `CR3` back to `g_kernel_pml4` prior to accessing physical memory addresses for ACPI parsing and hardware PMBASE I/O.
   - Timer interrupts and scheduler preemptions were active during the early shutdown phase, maintaining the user task address space.

---

### 3. Files Involved

1. `kernel/core/power/system_power.c`
   - Entry points: `atoms_power_shutdown()`, `atoms_power_reboot()`, `system_shutdown()`, `system_reboot()`.
   - Table parser: `find_acpi_rsdp()`, `find_acpi_fadt()`, `parse_s5_from_dsdt()`.
2. `kernel/shell/rook/src/rook_core.c`
   - Transition loop: `rook_shutdown_spin()`.

---

### 4. Risk Analysis

- **Regression Risk**: Extremely Low / Zero.
- Switching to `g_kernel_pml4` during shutdown and reboot restores the kernel's complete 512 GB physical identity map, allowing safe access to all UEFI configuration tables, ACPI tables, and hardware registers without altering normal desktop application execution or process isolation.
- Disabling interrupts (`cli`) right before bare-metal soft-off prevents race conditions where a timer interrupt switches `CR3` back to a user task mid-shutdown.

---

### 5. Suspected Fix Strategy (No Code)

1. **Kernel Address Space Restoration**:
   - In `system_power.c`, at the beginning of `atoms_power_shutdown()`, `atoms_power_reboot()`, `system_shutdown()`, and `system_reboot()`, query `vmm_get_kernel_pml4()` and switch the active `CR3` back to `g_kernel_pml4`.
2. **Interrupt Quiescing**:
   - In `system_shutdown()` and `system_reboot()`, execute `cli` immediately before performing ACPI table walks and hardware PMBASE I/O to guarantee atomic power transition without preemptive task switching.
3. **Table Traversal Bounds Validation**:
   - In `parse_s5_from_dsdt()`, enforce strict bounds checks against `dsdt->length` when parsing AML package bytes to prevent any potential out-of-bounds pointer increments.
4. **ACPI GUID Verification**:
   - In `find_acpi_rsdp()`, verify UEFI Configuration Table GUID signatures before dereferencing `vendor_table` pointers.
