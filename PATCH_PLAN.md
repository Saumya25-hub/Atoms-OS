# ATOMS OS — Architecture Patch Plan (Task 2)
## Subject: Surgical Patch Plan for Controlled Real Hardware NTFS Write Validation
**Input Document:** `FORENSIC_REPORT.md`  
**Target Hardware:** ASUS Prime B750M-K / Intel Core i3-14100F / Western Digital NVMe M.2 SSD  
**Target Partition:** Partition 3 (Start LBA: 239616, Size: 243 GB, Live Windows 11 NTFS)  
**Date:** 2026-09-04  
**Author:** Task 2 — Architect Team  
**Git Checkpoint Commit:** `32c280f92b7dfbe4efda762391264c8d50fe6154` (`checkpoint-nvme-read-pass`)

---

## 1. Architectural Strategy & Safety Tenets

1. **Zero Data Cluster Modification:**
   - The test content is strictly 105 bytes.
   - Under Microsoft NTFS architectural specifications, files under 256 bytes reside **exclusively inside the 1024-byte MFT record** as a Resident `$DATA` stream (`non_resident = 0`).
   - Consequently, the external cluster allocator (`ntfs_alloc_clusters`) will be completely bypassed for this test.
   - Zero clusters in the 243 GB data region will be allocated or written, eliminating any risk of cluster collision with Windows system files or user data.

2. **Zero In-Use MFT Record Overwrite:**
   - The hazardous hardcoded `s_next_free_record = 64` will be eliminated.
   - The allocator will dynamically scan MFT records starting strictly above the Windows reserved system region (`record >= 1024`).
   - It will verify that candidate records are completely free (`flags & NTFS_FILE_IN_USE == 0` and unallocated/zeroed).
   - Only a genuinely unused record will be assigned.

3. **Mandatory Pre-Existence Stop Gate:**
   - Before attempting any file creation, the driver will query the root directory.
   - If `ATOMS_WRITE_TEST.txt` already exists, the operation will immediately abort with a critical stop verdict.

4. **Surgical Root Directory Registration:**
   - Root directory MFT Record 5's resident `$INDEX_ROOT` attribute will be updated by inserting the new `NTFS_IndexEntry` before the End Marker (`NTFS_INDEX_ENTRY_LAST`), updating the index size, applying USA fixups, and flushing Record 5.
   - This ensures Windows 11 will cleanly recognize and list `C:\ATOMS_WRITE_TEST.txt` upon rebooting.

5. **Physical Hardware Clamping:**
   - All writes are channeled through sub-blockdevice `nvme0n1p3`, which enforces `lba + count <= sector_count`.
   - Partition tables (LBA 0..33) and EFI boot partitions (LBA 2048..206847) remain physically unreachable.

---

## 2. Files and Subsystems Scheduled for Modification

| File Path | Subsystem | Modification Scope |
| :--- | :--- | :--- |
| `kernel/vfs/vfs_legacy/fs/ntfs/src/ntfs.c` | NTFS Driver | Pre-existence gate, free MFT record scanner, resident data enforcement, `$INDEX_ROOT` entry insertion |
| `kernel/debug/storage_forensic_debug.c` | Diagnostic Runner | Execute controlled write test, flush, byte-for-byte read-back, ABDE dashboard display, UDP screenshot trigger |

*Note: Certified frozen subsystems (UEFI bootloader, VMM, PMM, BCM, ABDE renderer, USB HID, AHCI, NVMe read driver) will remain strictly untouched.*

---

## 3. Detailed Modification Plan

### Modification A: Pre-existence Gate in `ntfs.c`
- **Function:** `ntfs_create_file()`
- **Rationale:** Prevent overwriting or modifying any existing file on the volume.
- **Expected Result:** If `/ATOMS_WRITE_TEST.txt` is discovered in the root directory prior to creation, the function returns false and logs an explicit stop message.

### Modification B: Safe Dynamic MFT Record Allocation in `ntfs.c`
- **Function:** `ntfs_mft_alloc_record()`
- **Rationale:** Eliminate the hardcoded record 64 hazard which would corrupt an in-use Windows file.
- **Mechanism:**
  - Read candidate records starting at index 1024 up to the current MFT boundary.
  - Read the 1024-byte record from disk.
  - Verify that `(hdr->flags & NTFS_FILE_IN_USE) == 0` and the buffer is unallocated/zeroed.
  - Return the first verified free record number.
- **Expected Result:** A safe, genuine unallocated record number is selected without modifying any existing file's MFT record.

### Modification C: Strict Resident-Only Data Stream in `ntfs.c`
- **Function:** `ntfs_create_file()`
- **Rationale:** The test text is 105 bytes. Storing it as a resident attribute inside the new MFT record completely avoids touching the volume cluster bitmap (`$Bitmap`) or the data cluster area.
- **Mechanism:**
  - If `size <= 256`, bypass `ntfs_alloc_clusters()`. Set `clusters_needed = 0` and `alloc_lcn = 0`.
  - Copy data directly into the resident `$DATA` attribute value buffer inside the new MFT record.
- **Expected Result:** Zero clusters allocated in the data area; all file data is safely self-contained inside the new MFT record.

### Modification D: Root Directory `$INDEX_ROOT` Insertion in `ntfs.c`
- **Function:** `ntfs_btree_insert()`
- **Rationale:** Ensure the new file is formally registered in the directory index so that Windows 11 sees and opens it.
- **Mechanism:**
  - Locate `$INDEX_ROOT` in root directory Record 5.
  - Traverse entries to locate the End Marker (`NTFS_INDEX_ENTRY_LAST`).
  - Insert the new 88-byte `NTFS_IndexEntry` containing the filename `ATOMS_WRITE_TEST.txt` and file reference.
  - Shift the End Marker forward.
  - Update `IndexHeader->total_size` and `IndexHeader->allocated_size`.
  - Recompute USA fixup sequence for Record 5 and write to disk via `ntfs_write_mft_record_raw()`.
- **Expected Result:** Root directory properly indexes `ATOMS_WRITE_TEST.txt`.

### Modification E: Controlled Test Orchestration in `storage_forensic_debug.c`
- **Function:** `storage_forensic_debug_run()`
- **Rationale:** Perform the single, bounded write test, verify read-back, update ABDE diagnostics, and stream telemetry.
- **Mechanism:**
  - Confirm Phase 2 & 3 read validation succeeded.
  - Check if `/windows/ATOMS_WRITE_TEST.txt` exists. If so, abort.
  - Execute `ntfs_create_file()` with exact 105-byte deterministic string:
    ```
    ATOMS OS NTFS WRITE VALIDATION\nCreated by ATOMS on real hardware.\nTEST-ID: ATOMS-NTFS-WRITE-20260904\n
    ```
  - Call `nvme_flush()` to synchronize disk write buffers.
  - Re-open the file via `vfs_open()`, read 105 bytes, and perform byte-for-byte comparison.
  - If exact match, set verdict to `PASS — WRITE & READ-BACK VALIDATED`.
  - Trigger screenshot stream over UDP 9998 to capture write proof.

---

## 4. Risk Assessment & Safety Mitigations

| Identified Risk | Severity | Mitigation Strategy |
| :--- | :---: | :--- |
| Overwriting existing Windows file | **CRITICAL** | Pre-existence gate check; MFT candidate scanning verifies `flags & IN_USE == 0`; record index $\ge 1024$. |
| Corrupting volume cluster allocation | **HIGH** | Strict resident storage mode. Zero cluster allocation in data region. |
| Corrupting partition tables or bootloader | **CRITICAL** | Sub-device boundary clamping in `gpt_part_write()` enforces $lba + count \le sector\_count$. |
| Out-of-bounds directory index write | **HIGH** | Check available resident space in Record 5; abort if space is insufficient to fit the entry. |
| Incomplete write caching | **MEDIUM** | Issue low-level `NVME_IO_OP_FLUSH` command to controller immediately following write. |

---

## 5. Rollback Plan

If any step fails during testing or if read-back does not match:
1. Instantly stop execution; do not retry.
2. Revert workspace to Git checkpoint:
   ```bash
   git checkout checkpoint-nvme-read-pass
   ```
3. Rebuild clean read-only image via `build.ps1`.
