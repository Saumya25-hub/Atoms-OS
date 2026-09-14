# ATOMS OS — BOFS PHASE 10 PATCH PLAN
## BOSX / Execution Integration Architecture Plan

**Document ID:** `ATOMS-BOFS-PHASE10-PLAN-001`  
**Classification:** ARCHITECTURAL SPECIFICATION / PATCH PLAN  
**Date:** September 5, 2026  
**Status:** **PLAN APPROVED — READY FOR SURGICAL PATCHING**  
**Git Baseline Commit:** `300aba5` (`PHASE10_PRECHECKPOINT`)  

---

## 1. Architectural Scope & Target Files

In strict compliance with Rule 0 Phase Isolation, only the files explicitly listed below are authorized for modification or creation:

| File Path | Action | Architectural Rationale |
| :--- | :---: | :--- |
| `kernel/core/syscall/include/syscall.h` | Modify | Define `SYS_EXEC` (37) and declare `sys_service_exec` prototype. |
| `kernel/core/syscall/src/dispatcher.c` | Modify | Add `case SYS_EXEC:` routing to `sys_service_exec`. |
| `kernel/core/syscall/src/services.c` | Modify | Implement `sys_service_exec` with user pointer validation. |
| `kernel/core/loader/bosx_loader.h` | Modify | Add validation prototypes and return codes. |
| `kernel/core/loader/bosx_loader.c` | Modify | Implement full 9-stage production BOSX loader with W^X, checked arithmetic, and failure unwinding. Remove legacy ELF fallback. |
| `kernel/debug/bosx_phase10_test.h` | Create | In-kernel test header for Phase 10 verification. |
| `kernel/debug/bosx_phase10_test.c` | Create | In-kernel Phase 10 test suite and GOP 2560x1600 ABDE dashboard. |
| `kernel/kernel.c` | Modify | Define debug mode 14 (`ATOMS_DEBUG_MODE_BOFS_PHASE10`) and hook entry. |
| `build.ps1` | Modify | Add compilation step and link response line for `bosx_phase10_test.o`. |
| `tools/bofs/test_phase10_bosx.py` | Create | Host automated test suite (T01–T22, 1,000-cycle stress, INV-01..INV-15). |
| `tools/bofs/test_phase10_qemu.py` | Create | QEMU automated pure UEFI pre-flight test with screendump capture. |
| `docs/BOFS/PHASE10_BOSX_EXECUTION_INTEGRATION.md` | Create | Comprehensive 21-point technical specification document. |

---

## 2. Component Design & Pipeline Specifications

### A. BOSX Header & Section Validation Pipeline
1. **Header Validation**:
   - Verify `magic == 0x58534F42` (`BOSX_MAGIC`).
   - Verify `version_major == 1` and `version_minor == 0`.
   - Verify `architecture == 0x003E` (`BOSX_ARCH_X86_64`).
   - Verify `section_count > 0 && section_count <= 16`.
   - Verify `image_size > 0 && image_size <= 64 * 1024 * 1024`.
   - Verify `image_base >= 0x01000000ULL` and `image_base + image_size <= 0x7FFFFFFFFFFFULL`.
   - Verify no intersection with reserved kernel memory (`>= 0xFFFF800000000000`) or framebuffer (`0x80000000..0xD0000000`).
2. **Section Validation & Interval Disjointness**:
   - For every section $i$:
     - Check `(uint64_t)raw_data_offset + raw_data_size <= file_size`.
     - Check `(uint64_t)virtual_addr + virtual_size <= image_size`.
     - Check `raw_data_size <= virtual_size`.
     - If `flags & BOSX_SEC_BSS`: require `raw_data_size == 0`.
   - Pairwise overlap verification: For every pair $(i, j)$ with $i \neq j$:
     - Assert $[V_i, V_i + S_i) \cap [V_j, V_j + S_j) = \emptyset$.
3. **Entry Point Verification**:
   - Require that `entry_point` falls within $[V_k, V_k + S_k)$ where section $k$ has `flags & BOSX_SEC_EXEC`.
4. **W^X Enforcement**:
   - If `flags & BOSX_SEC_EXEC`, reject if `flags & BOSX_SEC_WRITE`.

### B. VFS & Permission Bridge
1. Resolve target executable path via VFS.
2. For each parent directory, verify `BOFS_PERM_EXEC` (search permission) using Phase 7 security.
3. For target file inode, verify `BOFS_PERM_EXEC` (execute permission).
4. If denied, fail-closed returning `EACCES` (-13).

### C. VMM Address Space & Mapping
1. Allocate new address space: `void *pml4 = vmm_create_address_space()`.
2. For each section:
   - Calculate page-aligned range: `start_page = (image_base + virtual_addr) & ~0xFFF`.
   - `end_page = (image_base + virtual_addr + virtual_size + 4095) & ~0xFFF`.
   - For each page:
     - Determine access flags: `VMM_ACCESS_USER | VMM_ACCESS_READ`.
     - If `flags & BOSX_SEC_WRITE`: `access |= VMM_ACCESS_WRITE`.
     - If `flags & BOSX_SEC_EXEC`: `access |= VMM_ACCESS_EXECUTE`.
     - Allocate page frame via `vmm_alloc_mapped_page(pml4, virt_page, access_flags)`.
     - Zero the entire 4KB frame.
     - If within `raw_data_size`, copy corresponding section bytes from file into physical frame.
3. User Stack Allocation:
   - Base address: `0x7FE00000ULL`, 32 pages (128 KB).
   - Top address: `0x7FE20000ULL`.
   - Access: `VMM_ACCESS_USER | VMM_ACCESS_READ | VMM_ACCESS_WRITE` (NX enforced).
   - Zero all stack pages.

### D. Process Creation & Task Spawning
1. Allocate PCB via `ATOMS_Process_Create(name, filepath, parent_pid, capabilities)`.
2. Assign `pcb->pml4_phys = (uint64_t)pml4`.
3. Fill `ProcessImage`:
   - `image.entry_point = image_base + header.entry_point;`
   - `image.image_base = image_base;`
   - `image.image_end = image_base + header.image_size;`
   - `image.stack_bottom = 0x7FE00000ULL;`
   - `image.stack_top = 0x7FE20000ULL - 8;`
   - `image.pml4 = pml4;`
   - `image.pid = pcb->pid;`
4. Spawn task via `process_spawn(&image, name)`.

### E. Defensive Rollback Architecture
On any failure during validation, allocation, or file reading:
- If `pml4` allocated: `vmm_destroy_address_space(pml4)`.
- If `pcb` created: `ATOMS_Process_Terminate(pcb->pid, -1)`.
- If file opened: `vfs_close(fd)`.
- Guarantee zero leakage of FDs, frames, inodes, or blocks.

---

## 3. Syscall Gateway Specification

### `SYS_EXEC` (Vector 37)
- Arguments: `a1 = const char *path`, `a2 = const char **argv`, `a3 = const char **envp`.
- Validation:
  - `path` validated via `syscall_validate_user_string(path, 256)`.
  - Rejects NULL, noncanonical, kernel pointers.
- Implementation:
  - Invokes `BOSX_LoadFromVFS(path, &child_pid)`.
  - Returns `child_pid` on success, or negative errno on failure (`-ENOENT`, `-EACCES`, `-ENOEXEC`, `-ENOMEM`).

---

## 4. Verification & Testing Strategy

1. **Host Test Suite (`tools/bofs/test_phase10_bosx.py`)**:
   - T01: Magic validation (`0x58534F42`)
   - T02: Header version validation
   - T03: Architecture validation (`0x003E`)
   - T04: Section bounds & count validation
   - T05: Integer overflow rejection
   - T06: Pairwise section overlap rejection
   - T07: Entry point inside executable section
   - T08: W^X permission enforcement
   - T09: Page alignment handling
   - T10: Truncated file rejection
   - T11: Checksum validation and corruption rejection
   - T12: BOFS execute permission check (`0755` vs `0644`)
   - T13: Directory traversal execution permission
   - T14: Fragmented BOFS file execution
   - T15: Zero-fill tail verification (no stale memory)
   - T16: User stack setup with NX
   - T17: Process creation & PCB binding
   - T18: Syscall pointer security enforcement
   - T19: Failure rollback (zero frame/PCB leak)
   - T20: Invalid file formats rejection (ELF, plain text, dir)
   - T21: Independent process address space isolation
   - T22: 1,000-cycle launch $\to$ execute $\to$ exit stress test
   - Invariants: Verify INV-01 to INV-15.
2. **QEMU Pre-Flight & Screendump**:
   - Boot pure UEFI, GOP 2560x1600, capture screendump, verify rotating heartbeat spinner, COM1 verdict `BOSX PHASE 10: CERTIFIED PASS`.
3. **Full Regression Matrix**:
   - Verify Phases 3, 4, 5, 6, 7, 8, 9 test suites continue to pass 100%.

---

## 5. Rollback Plan

If any critical regression occurs, restore working directory cleanly to `300aba5`:
```powershell
git checkout 300aba5 -- .
```

---
*Submitted by Architectural Engineering Team. Approved for Patch Implementation.*
