# ATOMS OS — BOFS PHASE 9: ARCHITECTURAL PATCH PLAN

**Document ID:** `ATOMS-BOFS-PHASE9-PLAN-001`  
**Phase:** TASK 2 — ARCHITECT TEAM  
**Standard:** Rule 0 Phase Isolation (Investigate ➔ Plan ➔ Patch ➔ Certify)  
**Date:** 2026-09-05  

---

## 1. Architectural Scope & Target Pipeline

Phase 9 establishes the complete userspace filesystem interface for ATOMS OS, integrating the certified BOFS engine into the ATOMS VFS and Syscall ABI:

```text
┌─────────────────────────────────────────────────────────────┐
│                    Userspace Application                    │
├─────────────────────────────────────────────────────────────┤
│               Syscall ABI (SYS_OPEN, SYS_READ...)           │
├─────────────────────────────────────────────────────────────┤
│         Syscall Gateway & Pointer Security Validator        │
├─────────────────────────────────────────────────────────────┤
│                 Virtual File System (VFS)                   │
│          (Mount Manager, Path Resolver, FD Table)           │
├─────────────────────────────────────────────────────────────┤
│                  BOFS Native VFS Driver                     │
│                  (bofs_fs_driver in VFS)                    │
├──────────────────────────────┬──────────────────────────────┤
│      BOFS Security (P7)      │        BOFS WAL (P8)         │
│  (bofs_check_permission)     │  (bofs_tx_begin / commit)    │
├──────────────────────────────┴──────────────────────────────┤
│                  BOFS Storage / Allocator                   │
├─────────────────────────────────────────────────────────────┤
│                   Block Device Subsystem                    │
└─────────────────────────────────────────────────────────────┘
```

---

## 2. Files to Create and Modify

### 2.1 Syscall Subsystem
- **Modify:** [`kernel/core/syscall/include/syscall.h`](file:///D:/Signatures_OS/kernel/core/syscall/include/syscall.h)
  - Add syscall numbers: `SYS_CREATE` (30), `SYS_MKDIR` (31), `SYS_READDIR` (32), `SYS_UNLINK` (33), `SYS_RENAME` (34), `SYS_RMDIR` (35), `SYS_STAT` (36).
  - Update `MAX_SYSCALL` to 40.
  - Define `atoms_stat_t` and `atoms_dirent_t` ABI structures.
- **Modify:** [`kernel/core/syscall/src/dispatcher.c`](file:///D:/Signatures_OS/kernel/core/syscall/src/dispatcher.c)
  - Add cases in `syscall_dispatch` for syscalls 30–36 routing to `sys_service_*`.
- **Modify:** [`kernel/core/syscall/src/services.c`](file:///D:/Signatures_OS/kernel/core/syscall/src/services.c)
  - Implement `sys_service_create`, `sys_service_mkdir`, `sys_service_readdir`, `sys_service_unlink`, `sys_service_rename`, `sys_service_rmdir`, `sys_service_stat`.
  - Validate all user string and buffer pointers using `syscall_validate_user_string` and `syscall_validate_user_ptr_writable`.

### 2.2 VFS Core Subsystem
- **Modify:** [`kernel/vfs/vfs_legacy/include/vfs_node.h`](file:///D:/Signatures_OS/kernel/vfs/vfs_legacy/include/vfs_node.h)
  - Widen `size` field in `VFS_Node` from `uint32_t` to `uint64_t`.
- **Modify:** [`kernel/vfs/vfs_legacy/include/vfs.h`](file:///D:/Signatures_OS/kernel/vfs/vfs_legacy/include/vfs.h)
  - Add `rmdir` and `stat` callback hooks to `FilesystemDriver`.
  - Declare `vfs_rmdir` and `vfs_stat` functions.
- **Modify:** [`kernel/vfs/vfs_legacy/src/vfs.c`](file:///D:/Signatures_OS/kernel/vfs/vfs_legacy/src/vfs.c)
  - Implement `vfs_rmdir` and `vfs_stat` functions dispatching to the target mount's filesystem driver.

### 2.3 BOFS VFS Driver Implementation
- **Create:** `kernel/vfs/bofs/include/bofs_vfs.h`
  - Declare `bofs_fs_driver`, `bofs_vfs_init`, and VFS context structures.
- **Create:** `kernel/vfs/bofs/src/bofs_vfs.c`
  - Implement driver callbacks: `mount`, `unmount`, `open`, `read`, `write`, `close`, `readdir`, `mkdir`, `create`, `rename`, `delete`, `rmdir`, `stat`.
  - Wire mount to Phase 8 recovery (`bofs_wal_mount` + `bofs_wal_recover`).
  - Wire open/read/write to Phase 7 security credentials and Phase 8 transactions.

### 2.4 Diagnostics & Verification
- **Create:** `tools/bofs/test_phase9_vfs_syscall.py`
  - Comprehensive automated test suite exercising all 25+ test cases, security matrices, pointer attacks, and crash recovery.
- **Create:** `kernel/debug/bofs_vfs_syscall_test.h` & `kernel/debug/bofs_vfs_syscall_test.c`
  - In-kernel diagnostic dashboard rendering GOP 2560x1600 UI and emitting COM1 serial telemetry for Phase 9 verification.
- **Modify:** [`kernel/kernel.c`](file:///D:/Signatures_OS/kernel/kernel.c)
  - Define `ATOMS_DEBUG_MODE_BOFS_PHASE9 13` and wire dispatch to `bofs_phase9_vfs_test_run(boot_info)`.
- **Modify:** [`build.ps1`](file:///D:/Signatures_OS/build.ps1)
  - Add `bofs_vfs.o` and `bofs_vfs_syscall_test.o` compilation and link entries.
- **Create:** `docs/BOFS/PHASE9_VFS_SYSCALL_UNIX_API_INTEGRATION.md`
  - Complete technical architecture and forensic documentation.

---

## 3. Expected Results

1. Real userspace syscall path operates end-to-end:
   `Userspace -> Syscall -> Pointer Check -> VFS -> BOFS -> BlockDevice`.
2. All invalid pointer attacks (NULL, kernel, unmapped, overflow) fail-closed with `SYSCALL_BAD_ADDRESS` without kernel panic or mutation.
3. Every mutating filesystem operation passes through Phase 8 WAL and Phase 7 security authorization.
4. Clean compilation with 0 warnings/errors under freestanding clang.
5. All regression suites (Phases 3–8) pass 100% with zero drift.

---

## 4. Rollback Plan

If any unexpected regression occurs, restore working tree to checkpoint commit `94736a7` using:
`git restore .` and `git clean -fd`.
