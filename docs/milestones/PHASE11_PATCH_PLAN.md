# ATOMS OS — BOFS Phase 11: File Manager Integration Patch Plan
**Document ID:** ATOMS-BOFS-PHASE11-PLAN-001  
**Phase:** 11 of BOFS Certification Series  
**Date:** 2026-09-05  
**Architect:** ATOMS Architecture Team (Task 2 — Specification Only, NO CODE)  
**Input:** `PHASE11_FORENSIC_REPORT.md`  
**Status:** READY FOR REVIEW & APPROVAL  

---

## 1. Architectural Objective

Seamlessly wire the existing ATOMS File Manager / Explorer to the real BOFS filesystem driver and Phase 10 Ring 3 BOSX execution pipeline through the unified VFS architecture.

**Core Principles:**
1. **Existing UI is 100% Preserved**: No modifications to toolbar, sidebar, layout, color themes, or window chrome.
2. **Canonical Pipeline Enforcement**:
   - `File Manager → VFS → BOFS → Phase 7 Security → Phase 8 WAL → BlockDevice`
   - `File Manager → SYS_EXEC (sys_service_exec) → VFS → BOFS → W^X Validation → VMM → Ring 3 Process`
3. **Phase Isolation Rule**: Task 3 will modify ONLY the files listed in Section 2 of this plan.

---

## 2. Permitted Files for Modification in Task 3

| Component | Exact File Path | Modification Summary |
|---|---|---|
| **Boot Sequence** | `kernel/kernel.c` | Register `bofs_vfs_init()` alongside `ntfs_init()` and `fat32_init()`. |
| **VFS Auto-Detection** | `kernel/vfs/vfs_legacy/src/vfs.c` | Add `BOFS_SUPER_MAGIC` (`0x53464F42U`) detection to `vfs_detect_fs()`. |
| **BOFS VFS Adapter** | `kernel/vfs/bofs/src/bofs_vfs.c` | Update `bofs_vfs_readdir_cb()` to stream single entries at `index` and populate `size`. |
| **File Explorer Host** | `kernel/shell/apps/explorer.c` | Wire executable double-click/open to `sys_service_exec()`, and Properties to `vfs_stat()`. |
| **File Explorer View** | `kernel/shell/apps/explorer_view.c` | Display "BOFS Native Volume" for BOFS volume cards. |
| **Phase 11 Kernel Test** | `kernel/debug/bofs_phase11_test.c` & `.h` | Add in-kernel File Manager/VFS/BOFS unit test suite. |
| **Phase 11 Host Test** | `tools/bofs/test_phase11_file_manager.py` | Standalone Python validation suite covering all 36 test matrix items (T01–T36). |
| **Build Configuration** | `build.ps1` | Compile `bofs_phase11_test.c` into build image. |

---

## 3. Surgical Modification Details

### 3.1 `kernel/vfs/bofs/src/bofs_vfs.c`
- **Function:** `bofs_vfs_readdir_cb()`
- **Why:** Current code passes `start_index = 0` with a 64-entry stack array, causing directories with $\ge 64$ entries to report EOF at index 63.
- **Expected Result:**
  - Pass `(uint32_t)index` to `bofs_sec_readdir()` with `max_count = 1`.
  - Check `bofs_inode_read()` to populate `out_entry->size` directly into `vfs_dirent_t`.
  - Directories of arbitrary size (leaf, router, multi-level B+Tree) enumerate completely without truncation.
- **Risk:** Low. `bofs_sec_readdir` already supports `offset_cookie`.
- **Rollback:** Restore original 64-entry chunk logic.

### 3.2 `kernel/vfs/vfs_legacy/src/vfs.c`
- **Function:** `vfs_detect_fs()`
- **Why:** `vfs_detect_fs()` only checks for NTFS and FAT32 signatures. BOFS drives are ignored during boot partition scans.
- **Expected Result:**
  - Read first 4 bytes of sector 0.
  - If `magic == 0x53464F42U` (`BOFS_SUPER_MAGIC`), return `"bofs"`.
  - Dynamic mount manager successfully auto-mounts BOFS partitions.
- **Risk:** Zero. Superblock magic `0x53464F42` is unique to BOFS.
- **Rollback:** Remove the 4-line check.

### 3.3 `kernel/kernel.c`
- **Location:** Lines ~688–694 (VFS boot initialization)
- **Why:** `bofs_vfs_init()` is never called in the kernel main boot flow.
- **Expected Result:**
  - Call `bofs_vfs_init();` after `vfs_init()` and alongside `fat32_init();` and `ntfs_init();`.
  - BOFS driver is fully registered in `filesystem_registry` before disk manager scans partitions.
- **Risk:** Zero. `bofs_vfs_init()` only calls `vfs_register_fs(&bofs_fs_driver)`.
- **Rollback:** Remove function call.

### 3.4 `kernel/shell/apps/explorer.c`
- **Functions:** `Explorer_HandleEvent()`, `Explorer_Refresh()`
- **Why:**
  1. Context menu "Properties" only displays path, ignoring `vfs_stat()`.
  2. Double-clicking or selecting "Open" on executables invokes dummy `BSOM_Invoke(item)`.
- **Expected Result:**
  1. Properties: Invokes `vfs_stat(target_path, &st)` and displays Inode, Size, Mode, and UID in the notification.
  2. Executable Launch: If target is `.bosx` or has execute permission, invokes `sys_service_exec(target_path, NULL, NULL)`. Displays success notification with spawned PID or controlled error.
  3. "This PC": If filesystem is `"bofs"`, displays "BOFS Storage (%s)".
- **Risk:** Low. `sys_service_exec()` was validated in Phase 10.
- **Rollback:** Revert `Explorer_HandleEvent()` to previous dispatch.

### 3.5 `kernel/shell/apps/explorer_view.c`
- **Function:** `Explorer_DrawFiles()` (This PC section)
- **Why:** Drive cards for BOFS volumes default to generic label "System Partition (A:)".
- **Expected Result:** Displays subtitle "BOFS Native Volume".
- **Risk:** Zero (display text only).
- **Rollback:** Revert string check.

---

## 4. Verification & Certification Strategy

1. **Host-Side Automated Suite (`tools/bofs/test_phase11_file_manager.py`)**:
   - 36 test cases covering T01 to T36:
     - T01–T03: Architecture & Ring audit
     - T04–T06: Syscall & BOFS mount & readdir
     - T07–T15: Complete CRUD lifecycle (open, read, create, mkdir, write, rename, stat, unlink, rmdir)
     - T16–T19: Navigation & Refresh external reconciliation
     - T20–T25: Permissions, Unicode, Case Sensitivity, Stale UI, Error codes
     - T26–T28: BOSX execution launch & invalid executable rejection
     - T29: Large directory traversal (> 64 items across B+Tree blocks)
     - T30: 1,000-cycle lifecycle stress test with zero drift
     - T31–T34: WAL crash recovery beneath File Manager
     - T35–T36: QEMU and real ATOMS integration
2. **Regression Verification**:
   - Run test suites for Phases 3 through 10. All must PASS.
3. **QEMU Pre-Flight**:
   - Build cleanly with `build.ps1` and run `test_phase10_qemu.py`.

---

## 5. Rollback Plan

If any regression or build failure occurs during Task 3:
```powershell
git checkout kernel/vfs/bofs/src/bofs_vfs.c
git checkout kernel/vfs/vfs_legacy/src/vfs.c
git checkout kernel/kernel.c
git checkout kernel/shell/apps/explorer.c
git checkout kernel/shell/apps/explorer_view.c
```
The workspace will immediately revert to the Phase 10 certified baseline (`76eff23`).
