# ATOMS OS — BOFS PHASE 10 FORENSIC INVESTIGATION REPORT
## BOSX / Execution Integration Architecture & Security Audit

**Document ID:** `ATOMS-BOFS-PHASE10-FORENSIC-001`  
**Classification:** FORENSIC AUDIT / KERNEL BRING-UP  
**Date:** September 5, 2026  
**Status:** **INVESTIGATION COMPLETE — NO CODE MODIFICATION PERFORMED**  
**Git Baseline Commit:** `300aba5` (`PHASE10_PRECHECKPOINT`)  

---

## 1. Executive Summary & Forensic Problem Statement

Phase 10 connects the certified BOFS storage subsystem (Phases 3–8) and VFS/Syscall gateway (Phase 9) to the ATOMS OS native executable platform: **BOSX**.

Forensic inspection of the existing codebase reveals that while the data structure specifications for BOSX (`BOSX_Header` and `BOSX_SectionHeader`) and the low-level Ring 3 task launching mechanism (`process_spawn`, `vmm_create_address_space`, `phase_b_jump_usermode`) exist, **no functional end-to-end execution pipeline currently connects VFS/BOFS file reads to process execution**. Specifically:
1. `kernel/core/loader/bosx_loader.c` contains only an early stub in `BOSX_LoadExecutableBuffer()` that creates a PCB entry, sets `pml4_phys = 0`, and ignores section headers, page mappings, stacks, and entry points.
2. `BOSX_Load()` contains legacy fallback logic that attempts to replace `.BOSX` extensions with `.ELF`, which violates the architectural mandate that BOSX is the native format and not an ELF clone.
3. No `SYS_EXEC` system call exists in `kernel/core/syscall/` to allow userspace processes to request execution of binary paths.
4. Execution authorization is not connected to Phase 7 `BOFS_PERM_EXEC` security checks during path traversal or inode evaluation.
5. Incomplete loads do not properly unwind resources, risking PML4 and frame leaks on malformed binaries.

---

## 2. Forensic Audit of BOSX Binary Specifications

### A. Ground Truth: `BOSX_Header` (Total Size: 128 Bytes)
Located in `kernel/core/loader/bosx_format.h`:

| Field | Offset | Size | Type | Meaning / Purpose | Strict Forensic Validation Rules |
| :--- | :---: | :---: | :--- | :--- | :--- |
| `magic` | 0 | 4 | `uint32_t` | Magic Identifier (`'BOSX'` = `0x58534F42`) | Must equal `0x58534F42` exactly. Never infer from extension. |
| `version_major` | 4 | 2 | `uint16_t` | Major Format Version | Must equal `1` (`BOSX_VERSION_MAJOR`). |
| `version_minor` | 6 | 2 | `uint16_t` | Minor Format Version | Must equal `0` (`BOSX_VERSION_MINOR`). |
| `abi_version` | 8 | 2 | `uint16_t` | Target OS ABI Version | Must equal `1` (`BOSX_ABI_VERSION`). |
| `architecture` | 10 | 2 | `uint16_t` | Machine Architecture | Must equal `0x003E` (`BOSX_ARCH_X86_64`). Reject 32-bit. |
| `entry_point` | 12 | 8 | `uint64_t` | Virtual Entry Point (RVA or VA) | Must lie inside a loaded executable (`BOSX_SEC_EXEC`) section. |
| `image_base` | 20 | 8 | `uint64_t` | Preferred Load Base Address | Must be page-aligned, in user space (`0x01000000`..`0x7FFFFFFF`). |
| `image_size` | 28 | 4 | `uint32_t` | Total Virtual Memory Size | Must be $> 0$, $\le 64\text{MB}$. `image_base + image_size` must not overflow. |
| `section_count`| 32 | 4 | `uint32_t` | Count of Section Headers | Must be $> 0$ and $\le 16$ (`BOSX_MAX_SECTIONS`). |
| `relocation_count` | 36 | 4 | `uint32_t` | Relocation Entry Count | Must fit within file bounds if present. |
| `import_count` | 40 | 4 | `uint32_t` | Import Entry Count | Must fit within file bounds if present. |
| `export_count` | 44 | 4 | `uint32_t` | Export Entry Count | Must fit within file bounds if present. |
| `checksum` | 48 | 4 | `uint32_t` | Header & Section Checksum | If non-zero, must match computed CRC32. |
| `build_id` | 52 | 4 | `uint32_t` | Unique Build Identifier | Retained for telemetry and debugging. |
| `signature` | 56 | 64 | `uint8_t[64]`| Digital Signature Placeholder | Checked if security profile requires it. |
| `reserved` | 120 | 16 | `uint32_t[4]`| Reserved for Future Expansion | Must be ignored or zero. |

### B. Ground Truth: `BOSX_SectionHeader` (Total Size: 36 Bytes)

| Field | Offset | Size | Type | Meaning / Purpose | Strict Forensic Validation Rules |
| :--- | :---: | :---: | :--- | :--- | :--- |
| `name` | 0 | 16 | `char[16]` | Section Name (`.text`, `.rodata`) | Bounded null-terminated ASCII string. |
| `virtual_addr` | 16 | 4 | `uint32_t` | Relative Virtual Address (RVA) | Must satisfy `virtual_addr + virtual_size <= image_size`. |
| `virtual_size` | 20 | 4 | `uint32_t` | Size in Virtual Memory | Must be $\ge raw\_data\_size$. |
| `raw_data_offset`| 24 | 4 | `uint32_t` | Byte Offset in File | `offset + size <= file_size`. No integer overflow. |
| `raw_data_size`| 28 | 4 | `uint32_t` | Size of Initialized Data | If BSS, must be 0. Otherwise $\le virtual\_size$. |
| `flags` | 32 | 4 | `uint32_t` | Permission & Attribute Flags | Valid mask of `BOSX_SEC_*`. W^X enforcement. |

Section Flags:
- `BOSX_SEC_READ` (`1 << 0`): Memory readable.
- `BOSX_SEC_WRITE` (`1 << 1`): Memory writable.
- `BOSX_SEC_EXEC` (`1 << 2`): Memory executable.
- `BOSX_SEC_BSS` (`1 << 3`): Uninitialized zero-filled section.
- `BOSX_SEC_RELOC` (`1 << 4`): Relocation table.

---

## 3. Discrepancy & Gap Analysis

1. **Documentation vs. Source Discrepancy**:
   - `doc/phase10_enterprise_native_platform.md` claimed a "64-byte structured header".
   - Ground Truth Source (`kernel/core/loader/bosx_format.h`) explicitly defines `BOSX_Header` as **128 bytes**.
   - **Resolution**: Adhere strictly to the source code (128 bytes).

2. **Absence of Section Loading**:
   - `bosx_loader.c` never iterates through `BOSX_SectionHeader`, never allocates pages via `vmm_alloc_mapped_page()`, and leaves `pcb->pml4_phys = 0`.
   - **Resolution**: Implement complete section parsing, page-aligned allocation, permission mapping, and zero-filling of tails and BSS.

3. **Absence of `SYS_EXEC`**:
   - The syscall table (`syscall.h`) currently supports up to syscall 36 (`SYS_STAT`).
   - There is no mechanism for a Ring 3 userspace process to request binary execution.
   - **Resolution**: Define `SYS_EXEC` as syscall 37 in `syscall.h` and route through `dispatcher.c` $\to$ `services.c`.

4. **Security & W^X Enforcement**:
   - VMM supports the hardware NX bit (`PAGE_NX = 1ULL << 63`).
   - Code segments (`BOSX_SEC_EXEC`) must have `PAGE_USER | PAGE_PRESENT` without `PAGE_WRITABLE` (or without `PAGE_NX`).
   - Writable data must have `PAGE_WRITABLE | PAGE_NX`.
   - Stacks must have `PAGE_WRITABLE | PAGE_NX`.
   - **Resolution**: Enforce strict W^X separation during page mapping.

5. **Failure Unwinding & Zero Drift**:
   - Malformed files or failed allocations must release any allocated PML4, physical frames, and PCB slots.
   - **Resolution**: Implement centralized failure cleanup in `bosx_loader.c`.

---

## 4. Evidence Matrix

| Subsystem | Existing File | Observed State | Required Phase 10 State |
| :--- | :--- | :--- | :--- |
| **BOSX Format** | `kernel/core/loader/bosx_format.h` | 128B header, 36B sections | Keep intact as source of truth |
| **BOSX Loader** | `kernel/core/loader/bosx_loader.c` | Incomplete stub; ELF fallback | Full 9-stage loader; zero ELF fallback |
| **Syscall ABI** | `kernel/core/syscall/include/syscall.h` | Up to syscall 36 (`SYS_STAT`) | Add `SYS_EXEC` (37) |
| **Syscall Dispatcher** | `kernel/core/syscall/src/dispatcher.c` | Cases 1..36 | Add case `SYS_EXEC` |
| **Syscall Services** | `kernel/core/syscall/src/services.c` | No `sys_service_exec` | Implement `sys_service_exec` |
| **VFS Integration** | `kernel/vfs/vfs_legacy/src/vfs.c` | Files opened via `vfs_open` | Check `BOFS_PERM_EXEC` on exec path |
| **Process Manager** | `kernel/core/process/process_manager.c` | PCB lifecycle working | Bind with `pml4_phys` teardown |

---

## 5. Suspected Risks & Failure Modes

1. **Integer Overflow on Section Offsets**:
   - Attackers could craft `raw_data_offset + raw_data_size` that wraps around 32-bit integers, bypassing `file_size` checks.
   - *Mitigation*: Use 64-bit checked arithmetic: `(uint64_t)offset + size <= file_size`.
2. **Overlapping Virtual Sections**:
   - Attackers could overlap a writable section onto an executable section to subvert W^X.
   - *Mitigation*: Perform pairwise $O(N^2)$ interval disjointness checks across all section headers.
3. **Invalid Entry Point**:
   - Entry point pointing to noncanonical memory, kernel memory, or outside loaded executable segments.
   - *Mitigation*: Strictly verify that `image_base + entry_point` falls within the span of a section flagged with `BOSX_SEC_EXEC`.
4. **Credential Forgery**:
   - Executable trying to claim UID 0 or special capabilities.
   - *Mitigation*: Process credentials must be inherited strictly from calling process context; ignore any capability claims in file metadata.
5. **Memory / Resource Drift on Malformed Binaries**:
   - If loading aborts after allocating 5 pages, those 5 pages could leak.
   - *Mitigation*: Invoke `vmm_destroy_address_space(pml4)` and clean PCB on every error exit path.

---

## 6. Suspected Fix Strategy (Phase Isolation Requirement: NO CODE)

1. Design `BOSX_ValidateHeader` and `BOSX_ValidateSections` with comprehensive defensive bounds checks and interval overlap verification.
2. Implement `BOSX_LoadFromVFS(const char *path, uint32_t *out_pid)`:
   - Traverses path through VFS.
   - Verifies Phase 7 `BOFS_PERM_EXEC` on all path directories and target inode.
   - Reads 128-byte `BOSX_Header` and $N \times 36$-byte `BOSX_SectionHeader`.
   - Validates all headers, checksums, and entry points.
   - Creates new isolated address space with `vmm_create_address_space()`.
   - Allocates user frames and maps sections with precise W^X permissions.
   - Zero-fills partial tails and BSS sections to eliminate stale physical memory exposure.
   - Allocates and zero-fills 128 KB user stack with `PAGE_NX`.
   - Spawns user task via `process_spawn()`.
3. Add `SYS_EXEC` (syscall 37) in `syscall.h`, `dispatcher.c`, and `services.c`.
4. Implement host test suite (`tools/bofs/test_phase10_bosx.py`) covering tests T01–T22 and invariants INV-01 to INV-15.
5. Implement in-kernel diagnostics dashboard (`kernel/debug/bosx_phase10_test.c`) for QEMU pure UEFI and bare-metal PXE validation.

---
*Submitted by Forensic Engineering Team. Awaiting Architectural Review and Approval.*
