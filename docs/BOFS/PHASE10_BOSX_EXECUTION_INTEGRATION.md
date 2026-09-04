# ATOMS OS — BOFS Phase 10: BOSX Execution Integration
**Document ID:** ATOMS-BOFS-PHASE10-DOC-001  
**Phase:** 10 of BOFS Certification Series  
**Status:** CERTIFIED PASS  
**Commit:** `76eff23`  
**Date:** 2026-09-05  

---

## 1. Objective

Phase 10 connects the certified BOFS filesystem engine (Phases 3–9) to the ATOMS native executable format (BOSX) and the full process execution pipeline.

The complete certified path is:

```
Ring 3 — SYS_EXEC (37)
  → sys_service_exec()          [syscall security / pointer validation]
  → BOSX_LoadFromVFS()          [VFS open + read]
  → BOSX_ValidateHeader()       [magic, version, arch, alignment]
  → BOSX_ValidateSections()     [W^X, overlap, bounds, entry check]
  → vmm_create_address_space()  [new PML4]
  → vmm_alloc_mapped_page()     [per section, W^X flags]
  → user stack (128 KB @ 0x7FE00000)
  → ATOMS_Process_Create()      [PCB, pml4_phys]
  → process_spawn()             [kernel stack, iretq frame, Ring 3]
  → scheduler_submit_task()     [live process]
  → SYS_EXIT → vmm_destroy_address_space() [cleanup]
```

---

## 2. BOSX Format (Source of Truth)

### BOSX_Header — 136 bytes

| Offset | Field | Type | Description |
|--------|-------|------|-------------|
| 0 | magic | uint32 | `0x58534F42` ('BOSX') |
| 4 | version_major | uint16 | Must be 1 |
| 6 | version_minor | uint16 | Minor version |
| 8 | abi_version | uint16 | ABI compatibility |
| 10 | architecture | uint16 | `0x003E` = x86_64 |
| 12 | entry_point | uint64 | Virtual entry RIP |
| 20 | image_base | uint64 | Base virtual address |
| 28 | image_size | uint32 | Total mapped size |
| 32 | section_count | uint32 | Number of sections (1–16) |
| 36 | relocation_count | uint32 | Relocation entries |
| 40 | import_count | uint32 | Import table entries |
| 44 | export_count | uint32 | Export table entries |
| 48 | checksum | uint32 | Image checksum |
| 52 | build_id | uint32 | Build identifier |
| 56 | signature | uint8[64] | Cryptographic signature field |
| 120 | reserved | uint32[4] | Reserved (zero) |

**Total: 136 bytes**

### BOSX_SectionHeader — 36 bytes

| Offset | Field | Type | Description |
|--------|-------|------|-------------|
| 0 | name | char[16] | Section name (null-padded) |
| 16 | virtual_addr | uint32 | Virtual address (image-relative) |
| 20 | virtual_size | uint32 | Size in virtual memory |
| 24 | raw_data_offset | uint32 | Byte offset in file |
| 28 | raw_data_size | uint32 | Size of raw data |
| 32 | flags | uint32 | READ=1, WRITE=2, EXEC=4, BSS=8, RELOC=16 |

### Section Constraints
- `BOSX_MAX_SECTIONS = 16`
- W^X enforced: `(EXEC | WRITE)` simultaneously rejected
- Sections must be pairwise disjoint in virtual address space
- Entry point must fall within an executable section
- BSS sections: `raw_data_size` must be 0

---

## 3. New Syscall: SYS_EXEC (37)

```c
// kernel/core/syscall/include/syscall.h
#define SYS_EXEC 37U
int64_t sys_service_exec(const char* path, uint64_t path_len);
```

Security enforcement:
1. Path pointer validated via `vmm_validate_user_range()`
2. Path length bounded to `VFS_MAX_PATH`
3. Phase 7 execute permission checked via `vfs_stat()` + mode bits
4. All failure paths release allocated resources (VMM frames, PCB)

---

## 4. BOSX Loader Architecture

### File: `kernel/core/loader/bosx_loader.c`

9-stage loading pipeline:

| Stage | Action |
|-------|--------|
| 1 | `vfs_stat()` — verify file exists, check execute permission |
| 2 | `vfs_open()` — open file descriptor |
| 3 | `vfs_read()` — read 136-byte header |
| 4 | `BOSX_ValidateHeader()` — magic, version, arch, alignment, address range |
| 5 | `vfs_read()` — read N×36-byte section headers |
| 6 | `BOSX_ValidateSections()` — W^X, bounds, overlap, BSS, entry point |
| 7 | `vmm_create_address_space()` — new PML4, kernel entries copied |
| 8 | Per-section: `vmm_alloc_mapped_page()` + `vfs_read()` + zero-fill tail |
| 9 | User stack allocation, `ATOMS_Process_Create()`, `process_spawn()` |

### Error Codes

| Code | Value | Meaning |
|------|-------|---------|
| BOSX_OK | 0 | Success |
| BOSX_ERR_BAD_MAGIC | -1 | Invalid magic |
| BOSX_ERR_BAD_VERSION | -2 | Unsupported version |
| BOSX_ERR_BAD_ARCH | -3 | Wrong architecture |
| BOSX_ERR_BAD_SECTIONS | -4 | Section table invalid |
| BOSX_ERR_NO_EXEC | -5 | No executable section |
| BOSX_ERR_VMM | -6 | VMM allocation failed |
| BOSX_ERR_PROCESS | -7 | Process creation failed |
| BOSX_ERR_VFS | -8 | VFS operation failed |
| BOSX_ERR_OVERFLOW | -9 | Integer overflow detected |
| BOSX_ERR_OVERLAP | -10 | Section address overlap |
| BOSX_ERR_BAD_ENTRY | -11 | Entry point not in exec section |
| BOSX_ERR_CORRUPT | -12 | File data corrupt |
| BOSX_ERR_PERMISSION | -13 | Execute permission denied |

---

## 5. W^X Enforcement

Write XOR Execute is enforced at two levels:

1. **Validation**: `BOSX_ValidateSections()` rejects any section with both `EXEC` and `WRITE` flags
2. **Mapping**: `vmm_alloc_mapped_page()` called with mutually exclusive flags:
   - Executable sections: `PAGE_USER` (no `PAGE_WRITABLE`)
   - Writable sections: `PAGE_USER | PAGE_WRITABLE` (no execute)
   - Stack: `PAGE_USER | PAGE_WRITABLE` (NX by default on x86_64 without explicit exec flag)

---

## 6. Process Lifecycle

```
Bosx_LoadFromVFS(path)
  └─ vmm_create_address_space()      → new_pml4
  └─ section mapping loop
  └─ user stack: 128 KB @ 0x7FE00000
  └─ ATOMS_Process_Create()          → PCB (pcb->pml4_phys = new_pml4)
  └─ process_spawn(&img, name)       → kernel stack + iretq frame
       CS=0x23, SS=0x1B, RFLAGS=0x202, RSP=stack_top, RIP=entry
  └─ scheduler_submit_task()         → process is live

SYS_EXIT
  └─ scheduler_terminate_task()
  └─ ATOMS_Process_Terminate()
  └─ clear_pcb_locked()              → vmm_destroy_address_space(pml4_phys)
  └─ scheduler_reap_terminated_tasks() → free kernel stack + task struct
```

---

## 7. Test Suite Results

### Host Test Suite: `tools/bofs/test_phase10_bosx.py`

| Test | Description | Result |
|------|-------------|--------|
| T01 | BOSX Magic Validation | PASS |
| T02 | Header Version Validation | PASS |
| T03 | Architecture Validation (Reject 32-bit) | PASS |
| T04 | Section Bounds & Count Validation | PASS |
| T05 | Integer Overflow Rejection | PASS |
| T06 | Pairwise Section Overlap Rejection | PASS |
| T07 | Entry Point Validation | PASS |
| T08 | Permissions & W^X Enforcement | PASS |
| T09 | Page Alignment Handling | PASS |
| T10 | Truncated Executable Rejection | PASS |
| T11 | Corrupted Executable Rejection | PASS |
| T12 | BOFS Execute Permission Check | PASS |
| T13 | Directory Traversal Search Permission | PASS |
| T14 | Fragmented BOFS File Execution | PASS |
| T15 | Zero-Fill Tail & BSS Verification | PASS |
| T16 | User Stack Setup with NX | PASS |
| T17 | Process Creation & PCB Binding | PASS |
| T18 | Syscall Pointer Security Enforcement | PASS |
| T19 | Failure Rollback (Zero Frame/PCB Leak) | PASS |
| T20 | Invalid Executable Rejection | PASS |
| T21 | Independent Process Address Space Isolation | PASS |
| T22 | 1,000-Cycle Lifecycle Stress Test | PASS |
| INV-01–15 | Formal Invariants | PASS |

**Total: 37/37 PASS**

### QEMU Pre-Flight: `tools/bofs/test_phase10_qemu.py`

- **Resolution:** 2560×1600 GOP
- **Boot time to PASS:** 20 seconds
- **All 20 in-kernel verifications:** PASS
- **Resource drift:** FRAME=0, PROCESS=0, FD=0, INODE=0, BLOCK=0
- **Kernel panics:** 0
- **Foreign storage:** WRITE LOCKED (0 bytes touched)

### Phase 3–9 Regression: `tools/bofs/test_phase9_vfs_syscall.py`

- **24/24 PASS — Zero regressions**

---

## 8. Safety Invariants

| Invariant | Verification |
|-----------|--------------|
| Invalid BOSX cannot execute | Header + section validation rejects before any VMM allocation |
| Entry point in executable section | Checked by `BOSX_ValidateSections()` |
| BOSX cannot map kernel memory | `vmm_validate_user_range()` + VMM_USER_MIN/MAX bounds |
| BOSX cannot forge UID/GID | Loader inherits caller's security context from Phase 7 |
| Execute permission enforced | `vfs_stat()` mode bit check before VFS open |
| Failed load = zero resource leak | All VMM frames freed on any error path |
| Process exit restores baseline | `clear_pcb_locked()` → `vmm_destroy_address_space()` |
| Foreign storage untouched | WRITE LOCKED throughout |

---

## 9. What BOSX Is Not

BOSX is the **native ATOMS executable format**. It is explicitly:
- NOT ELF
- NOT PE/EXE
- NOT a Linux binary
- NOT a Windows binary

UNIX/POSIX/Linux/Windows executable models are referenced only as behavioral inspiration. BOSX architecture, layout, and semantics are independent ATOMS/BOS designs.

---

## 10. Certification Record

| Item | Value |
|------|-------|
| Phase | 10 — BOSX Execution Integration |
| Commit | `76eff23` |
| Branch | `stable/v2.6-gpu-and-mouse-engine` |
| Previous Phase Commit | `300aba5` (Phase 9) |
| Host Tests | 37/37 PASS |
| QEMU Pre-Flight | PASS (2560×1600, 20s boot) |
| Phase 3–9 Regression | 24/24 PASS |
| Resource Drift | 0 across all categories |
| Kernel Panics | 0 |
| Foreign Storage | WRITE LOCKED |
| Physical BOFS Volume | NOT AVAILABLE |
| Status | **CERTIFIED PASS** |
