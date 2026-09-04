# ATOMS OS — BOFS PHASE 9: FORENSIC INVESTIGATION REPORT

**Document ID:** `ATOMS-BOFS-PHASE9-FORENSIC-001`  
**Phase:** TASK 1 — FORENSIC TEAM  
**Target:** VFS + Syscall + UNIX-Style API Integration  
**Standard:** Rule 0 Phase Isolation (Investigate ➔ Plan ➔ Patch ➔ Certify)  
**Date:** 2026-09-05  

---

## 1. Executive Summary & Forensic Objective

Phases 3 through 8 established and certified the underlying freestanding BOFS storage engine: on-disk structures (Phase 3), block allocation (Phase 4), metadata & file lifecycle (Phase 5), B+Tree directory namespace (Phase 6), UID/GID/mode security permissions (Phase 7), and write-ahead logging with crash consistency (Phase 8).

Phase 9 establishes the production path connecting actual userspace applications to BOFS through the ATOMS Syscall ABI, pointer validation layer, and Virtual File System (VFS):
$$\text{Userspace} \xrightarrow{\text{SYSCALL}} \text{Syscall Gateway} \xrightarrow{\text{Pointer Check}} \text{VFS} \xrightarrow{\text{FilesystemDriver}} \text{BOFS} \xrightarrow{\text{WAL}} \text{BlockDevice}$$

---

## 2. Syscall Table Audit (Existing State vs Required)

| Syscall Name | Number | Current State | Target Phase 9 Implementation | Security Validation |
|:---|:---:|:---|:---|:---|
| `SYS_OPEN` | 14 | Exists in `services.c` (`sys_service_open`) | Route to `vfs_open(path)` | `syscall_validate_user_string(path, 256)` |
| `SYS_READ` | 15 | Exists in `services.c` (`sys_service_read`) | Route to `vfs_read(fd, buf, count)` | `syscall_validate_user_ptr_writable(buf, count)` |
| `SYS_CLOSE` | 25 | Exists in `services.c` (`sys_service_close`) | Route to `vfs_close(fd)` | Validates FD range `[3, MAX_OPEN_FILES)` |
| `SYS_SEEK` | 26 | Exists in `services.c` (`sys_service_seek`) | Route to `vfs_seek(fd, offset, whence)` | Bounds & negative offset checking |
| `SYS_WRITE_FILE` | 29 | Exists in `services.c` (`sys_service_write_file`) | Route to `vfs_write(fd, buf, count)` | `syscall_validate_user_ptr(buf, count)` |
| `SYS_CREATE` | 30 | Missing | New: `sys_service_create(path, mode)` | `syscall_validate_user_string(path, 256)` |
| `SYS_MKDIR` | 31 | Missing | New: `sys_service_mkdir(path, mode)` | `syscall_validate_user_string(path, 256)` |
| `SYS_READDIR` | 32 | Missing | New: `sys_service_readdir(path, idx, dirent)` | `syscall_validate_user_string` + `syscall_validate_user_ptr_writable` |
| `SYS_UNLINK` | 33 | Missing | New: `sys_service_unlink(path)` | `syscall_validate_user_string(path, 256)` |
| `SYS_RENAME` | 34 | Missing | New: `sys_service_rename(old, new)` | `syscall_validate_user_string` for both |
| `SYS_RMDIR` | 35 | Missing | New: `sys_service_rmdir(path)` | `syscall_validate_user_string(path, 256)` |
| `SYS_STAT` | 36 | Missing | New: `sys_service_stat(path, stat_buf)` | `syscall_validate_user_string` + `syscall_validate_user_ptr_writable` |

---

## 3. VFS Layer Audit & Gap Analysis

1. **`VFS_Node.size` width**: Currently defined as `uint32_t size;` in `kernel/vfs/vfs_legacy/include/vfs_node.h`. This risks truncation on files > 4 GB. Must be safely widened to `uint64_t size;`.
2. **`FilesystemDriver`**: Currently includes `mount`, `unmount`, `open`, `read`, `write`, `close`, `readdir`, `mkdir`, `create`, `rename`, `delete`. Lacks `rmdir` and `stat` callbacks.
3. **Driver Registration**: BOFS must register as `"bofs"` via `vfs_register_fs()`.
4. **Mount Model**: Existing `vfs_mount_fs()` handles mount registration into `mount_table`. The BOFS mount callback must validate the superblock, invoke Phase 8 WAL recovery (`bofs_wal_mount` + `bofs_wal_recover`), and fail closed if recovery indicates corruption.
5. **FD Table Management**: Global table `g_fd_table[32]` with FDs starting at 3 (0, 1, 2 reserved for standard streams). `vfs_open` creates a private `VFS_Node`, attaches filesystem context, and `vfs_close` frees the node and clears `in_use`.

---

## 4. User Pointer Security Analysis

The existing validation functions in `kernel/core/syscall/src/validation.c`:
- `syscall_validate_user_ptr(ptr, size)`
- `syscall_validate_user_ptr_writable(ptr, size)`
- `syscall_validate_user_string(str, max_len)`
verify canonical addresses, non-NULL, range bounded within usermode window `[USER_WINDOW_MIN, USER_WINDOW_MAX)` (`[0x40000000, 0x80000000)`), and validates active PML4 page tables for `PAGE_USER` and write permissions.

Any invalid pointer will return `SYSCALL_BAD_ADDRESS` without panicking or modifying the filesystem.

---

## 5. Security & WAL Enforcement Boundary

1. **Authorization Authority**: Phase 7 (`bofs_security.c`) remains authoritative. The VFS and syscall layers MUST NOT invent ad-hoc permissions; all calls resolve to `bofs_sec_*` functions with credentials from current process context.
2. **Crash Consistency Authority**: Phase 8 (`bofs_wal.c`) remains authoritative. All mutating operations (`create`, `write`, `mkdir`, `unlink`, `rename`, `rmdir`) must execute within a formal `bofs_tx_begin` / `bofs_tx_commit` transaction.

---

## 6. Risk Analysis & Mitigation

| Risk | Consequence | Mitigation |
|:---|:---|:---|
| Unbounded `strlen` on user pointer | Kernel memory disclosure / Page fault | `syscall_validate_user_string` with max bound 256 bytes |
| Invalid FD in `read`/`write`/`close` | Kernel pointer dereference | Validate `fd >= 3 && fd < MAX_OPEN_FILES && g_fd_table[fd].in_use` |
| Concurrent open of same file | State collision | Each FD instantiates its own `VFS_Node` and offset |
| Partial/torn metadata update | Filesystem corruption | Enforce Phase 8 WAL transaction on every mutation |
| Mount of corrupted volume | System instability | Mount verifies journal and superblock; fails closed on error |
