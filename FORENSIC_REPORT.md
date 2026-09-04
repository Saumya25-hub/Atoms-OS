# ATOMS OS — BOFS Phase 11: File Manager Forensic Audit Report
**Document ID:** ATOMS-BOFS-PHASE11-FORENSIC-001  
**Phase:** 11 of BOFS Certification Series  
**Date:** 2026-09-05  
**Investigator:** ATOMS Forensic Team (Task 1 — 100% Read-Only Audit)  
**Status:** FORENSIC AUDIT COMPLETE — ROOT CAUSES & GAPS IDENTIFIED (NO CODE)  

---

## 1. Executive Summary

A comprehensive forensic audit of the ATOMS OS File Manager / Explorer subsystem was conducted to determine whether the user interface is driven by the real `Ring 3 → Syscall → VFS → BOFS` stack or by fake, hardcoded in-memory fixtures.

### Key Conclusions:
1. **The Production File Manager UI is REAL, not fake**:
   - The visible desktop File Explorer (`kernel/shell/apps/explorer.c`, `explorer_view.c`) derives its "This PC" drive list from dynamic VFS queries (`vfs_get_mount_count()`, `vfs_get_mount_info()`).
   - Folder contents are enumerated dynamically through `vfs_readdir(path, index, &dirent)`.
   - File creation, folder creation, deletion, and renaming directly invoke `vfs_create()`, `vfs_mkdir()`, `vfs_delete()`, and `vfs_rename()`.
   - Text file viewing/editing launches `notes_app.c`, which reads and writes persisted data via `vfs_open()`, `vfs_read()`, and `vfs_write()`.
2. **Fake Data Exists Only in Standalone Test Fixtures**:
   - `userspace/apps/fileexplorer/` contains hardcoded test fixtures (`fe_drives.c`, `fe_listview.c`) created for a mock 600-test certification harness (`fe_certification_tests.c`). These are preserved as test fixtures per Section 3 instructions and are not what the user interacts with in the desktop shell.
3. **Four Critical Architectural Gaps / Bugs Discovered**:
   - **Bug 1**: `bofs_vfs_init()` is never invoked during system startup in `kernel/kernel.c`. The BOFS filesystem driver is unregistered with VFS on normal boots.
   - **Bug 2**: `vfs_detect_fs()` in `kernel/vfs/vfs_legacy/src/vfs.c` lacks BOFS superblock detection (`0x53464F42U`), preventing auto-discovery of BOFS partitions.
   - **Bug 3**: `bofs_vfs_readdir_cb()` in `kernel/vfs/bofs/src/bofs_vfs.c` hardcodes a 64-entry buffer and passes `start_index = 0`, truncating any directory with $\ge 64$ entries at index 63.
   - **Bug 4**: Double-clicking executable files or selecting "Open" in File Explorer calls `BSOM_Invoke(item)` (an empty stub) rather than dispatching through the Phase 10 `SYS_EXEC` / `sys_service_exec()` pipeline to launch Ring 3 BOSX processes.
   - **Bug 5**: File "Properties" in context menu displays only the path string and never queries `vfs_stat()` for inode, mode, size, and timestamps.

---

## 2. File Manager Source Map

```text
┌────────────────────────────────────────────────────────────────────────┐
│                   FILE MANAGER SOURCE & CONTROL MAP                    │
├────────────────────────────────┬──────────┬──────────────┬─────────────┤
│ File Path                      │ Ring Lvl │ Function     │ Target VFS  │
├────────────────────────────────┼──────────┼──────────────┼─────────────┤
│ kernel/shell/apps/explorer.c   │ Ring 0   │ UI Host/Ctrl │ VFS & BSOM  │
│ kernel/shell/apps/explorer_view│ Ring 0   │ Pure Draw    │ None (View) │
│ kernel/shell/apps/notes_app.c  │ Ring 0   │ Text Editor  │ VFS File IO │
│ kernel/shell/apps/rename_dialog│ Ring 0   │ Modal Dialog │ vfs_rename  │
│ kernel/vfs/bofs/src/bofs_vfs.c │ Kernel   │ VFS Adapter  │ BOFS Driver │
│ kernel/core/loader/bosx_loader │ Kernel   │ Exec Loader  │ VFS -> VMM  │
│ kernel/core/syscall/src/*      │ Kernel   │ Syscall ABI  │ VFS Services│
└────────────────────────────────┴──────────┴──────────────┴─────────────┘
```

---

## 3. Forensic Defect Classification

### 🔴 Bug 1: BOFS Driver Unregistered at Boot
- **Location:** `kernel/kernel.c:688–694`
- **Observed:** `kernel.c` registers `dummyfs`, `fat32`, and `ntfs`, but omits `bofs_vfs_init()`.
- **Root Cause:** BOFS integration from Phase 9 was tested in isolated debug harnesses and never added to the primary boot sequence.
- **Impact:** BOFS partitions cannot be mounted or accessed by the File Manager on normal boot.
- **Risk:** High.
- **Suspected Fix:** Declare `extern void bofs_vfs_init(void);` and call it during VFS initialization in `kernel.c`.

### 🔴 Bug 2: Missing BOFS Detection in `vfs_detect_fs()`
- **Location:** `kernel/vfs/vfs_legacy/src/vfs.c:293–327`
- **Observed:** `vfs_detect_fs()` inspects sector 0 for NTFS OEM string and FAT32 boot parameters, returning `NULL` for any BOFS storage volume.
- **Root Cause:** Detection logic was never updated for BOFS superblock magic (`BOFS_SUPER_MAGIC = 0x53464F42U`).
- **Impact:** The VFS auto-mount loop ignores all physical or virtual BOFS drives.
- **Risk:** High.
- **Suspected Fix:** Check if `buffer[0..3] == 0x53464F42U`, return `"bofs"`.

### 🔴 Bug 3: Hardcoded 64-Entry Limit in `bofs_vfs_readdir_cb()`
- **Location:** `kernel/vfs/bofs/src/bofs_vfs.c:331–348`
- **Observed:**
  ```c
  bofs_dirent_t dirents[64];
  r = bofs_sec_readdir(&mctx->fs, dir_ino, &cred, 0, dirents, 64, &count);
  if (index >= 0 && (uint32_t)index < count) { ... }
  ```
- **Root Cause:** `bofs_sec_readdir` supports seeking via `offset_cookie` (start index), but `bofs_vfs_readdir_cb` passed `0` and a fixed stack array of 64 entries.
- **Impact:** Any folder with $> 64$ entries reports EOF at item 63. Large directories cannot be enumerated.
- **Risk:** High.
- **Suspected Fix:** Pass `(uint32_t)index` as `offset_cookie` and `max_count = 1` to `bofs_sec_readdir()`. Populate `out_entry->size` from inode metadata.

### 🔴 Bug 4: BOSX Application Execution Disconnected from File Manager
- **Location:** `kernel/shell/apps/explorer.c:263, 450`
- **Observed:** Double-clicking or opening non-text files routes to `BSOM_Invoke(item)`, which returns 0 without action.
- **Root Cause:** Explorer was never wired to the Phase 10 execution loader (`sys_service_exec()` / `BOSX_LoadFromVFS()`).
- **Impact:** Double-clicking `.bosx` executables does nothing.
- **Risk:** High.
- **Suspected Fix:** Route double-click and context menu Open to `sys_service_exec(target_path, NULL, NULL)`. On success, notify user with PID; on failure, notify graceful error (no kernel panic).

### 🔴 Bug 5: Properties Dialog Lacks Real Filesystem Metadata
- **Location:** `kernel/shell/apps/explorer.c:280–284`
- **Observed:** Right-click -> "Properties" shows only the raw path string.
- **Root Cause:** Never invokes `vfs_stat()` or `SYS_STAT`.
- **Impact:** Inode number, file size, permissions, and timestamps are not exposed.
- **Risk:** Medium.
- **Suspected Fix:** Invoke `vfs_stat(target_path, &st)` and format a diagnostic string with real metadata.

---

## 4. Invariant Audit Matrix

| Invariant | Requirement | Current State | Verdict |
|---|---|---|---|
| **INV-01** | Real VFS Directory Enumeration | `vfs_readdir` called in loop | 🟢 PASS |
| **INV-02** | Real Mount Discovery | `vfs_get_mount_info` called | 🟢 PASS |
| **INV-03** | No Synthesized Fake Folders in Production | No `SYS32` or `APPS` synthesized | 🟢 PASS |
| **INV-04** | Large Directory Unbounded Traversal | Truncated at 64 entries | 🔴 BUG (Fix in Task 3) |
| **INV-05** | BOFS VFS Registration | Uncalled at boot | 🔴 BUG (Fix in Task 3) |
| **INV-06** | BOFS Auto-Detection | Missing in `vfs_detect_fs` | 🔴 BUG (Fix in Task 3) |
| **INV-07** | BOSX Launch through `SYS_EXEC` | `BSOM_Invoke` stub | 🔴 BUG (Fix in Task 3) |
| **INV-08** | Stat Metadata Integration | Missing in Properties | 🔴 BUG (Fix in Task 3) |
| **INV-09** | Existing UI Preservation | UI/layout intact | 🟢 PASS |
| **INV-10** | Foreign Storage Writes = 0 Bytes | Write locks intact | 🟢 PASS |

---

## 5. Next Steps
Proceed to Task 2 (Architecture Plan — `PHASE11_PATCH_PLAN.md` / `PATCH_PLAN.md`) upon review.
