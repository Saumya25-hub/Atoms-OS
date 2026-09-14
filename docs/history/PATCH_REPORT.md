# PATCH REPORT: VMM Usermode Page Directory Isolation & MMap Safety

**Date:** 2026-09-13  
**Status:** COMMITTED  
**Protocol Phase:** TASK 3 (PATCH TEAM)  

---

## 1. Files Changed

### 1. `kernel/core/memory/vmm/src/vmm.c`
- **Functions Changed**:
  - `vmm_create_address_space()`:
    - Replaced the copy-loop that populated `user_pd1` from `k_pd1` with a clean zero initialization (`for (int i = 0; i < 512; i++) user_pd1[i] = 0;`).
    - Prevents user processes from inheriting kernel 2MB huge pages or dynamically split Page Tables (such as `0x0C72A000` created during `vmm_init` Stress Test 3).
  - `vmm_destroy_address_space()`:
    - Added a safety check verifying that dynamic Page Tables in the teardown process do not match any Page Table referenced by the kernel's `k_pdp` / `k_pd`.
    - Prevents kernel-owned Page Tables from ever being marked as free frames in `g_pmm_bitmap`.
  - Removed temporary debug printouts around `0x60011000`.

### 2. `kernel/core/syscall/src/services.c`
- **Functions Changed**:
  - `sys_service_mmap()`:
    - Initialized and bounds-checked `s_user_mmap_bump` at `0x60000000ULL` (1.5 GB), completely isolating usermode dynamic heap allocations from Window Canvas surfaces (`0x50000000ULL..0x5F000000ULL`).
    - Derived `target_pml4` from `hw_cr3 & PAGE_PHYS_ADDRESS_MASK`.
    - Zeroed physical page frame directly via physical RAM identity mapping `memset((void*)phys, 0, 4096)` before mapping.
    - Preserved `vmm_walk_and_verify(target_pml4, virt_start)` for forensic confirmation.

---

## 2. Quantitative Summary

- `kernel/core/memory/vmm/src/vmm.c`: ~28 lines modified/removed.
- `kernel/core/syscall/src/services.c`: ~15 lines modified.
- Unrelated subsystems touched: 0.
- `kernel.c` modifications: 0.
