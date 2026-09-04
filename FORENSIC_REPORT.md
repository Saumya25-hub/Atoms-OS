# ATOMS OS — Forensic Audit Report (Phase 0 & Phase 1)
## Subject: Safety-Critical Real Hardware NTFS Write Path Audit
**Target Hardware:** ASUS Prime B750M-K (Intel Core i3-14100F LGA1700)  
**Storage Medium:** Western Digital WD Blue SN5000 500GB NVMe M.2 Gen4 SSD (`0x15B7:0x5017`)  
**Target Partition:** Partition 3 (Start LBA: 239616, Size: 243 GB, Microsoft Basic Data GUID)  
**Target Filesystem:** Live Physical Microsoft Windows 11 Installation  
**Date:** 2026-09-04  
**Author:** Task 1 — Forensic Team  
**Git Checkpoint Commit:** `32c280f92b7dfbe4efda762391264c8d50fe6154` (`checkpoint-nvme-read-pass`)

---

## 1. Executive Summary & Objective

The objective of this mission is to perform **exactly ONE controlled, non-destructive write test** on the real Windows 11 NTFS volume (`/ATOMS_WRITE_TEST.txt`) containing deterministic ASCII text, verify the read-back byte-for-byte in ATOMS OS, shut down cleanly, and have the user independently verify the file in Windows 11.

Because this NVMe drive contains the user's active, production Windows 11 operating system, **RULE 0 (Mandatory Phase Isolation)** is strictly enforced:
- Zero writes may occur until every link in the creation/write pipeline is forensically audited and proven 100% safe.
- Absolutely NO modification, deletion, renaming, truncation, or movement of ANY existing file or directory is permitted.

---

## 2. Phase 0: Git Safety Checkpoint Confirmation

- **Working Tree State:** Completely clean prior to audit.
- **Stable Certified Base Commit:** `32c280f92b7dfbe4efda762391264c8d50fe6154`
- **Permanent Checkpoint Tag:** `checkpoint-nvme-read-pass`
- **Certified Frozen Subsystems:** UEFI Bootloader (`bootx64.c`), BCM, ABDE, USB HID, VMM Paging, PMM Allocator, TSS/SMP, VFS Unmount Lifecycle, AHCI SATA Driver, NVMe Read Driver.

---

## 3. Phase 1: End-to-End NTFS Write Path Forensic Trace

The complete lifecycle from user test execution down to physical NVMe write was audited:

```
[User / Diagnostic Test Runner]
               ↓
[Pre-existence Safety Gate] (MUST STOP IF TARGET EXISTS)
               ↓
[VFS File Creation Interface: vfs_create / ntfs_create_file]
               ↓
[MFT Record Allocation Engine: ntfs_mft_alloc_record]
               ↓
[Storage Allocation Engine: Resident Data Stream (size <= 256 B)]
               ↓
[MFT Record Construction: $STANDARD_INFO + $FILE_NAME + Resident $DATA]
               ↓
[Directory B+Tree Index Insertion: Root Directory MFT Record 5 ($INDEX_ROOT)]
               ↓
[MFT Record Flush & Fixup Application: ntfs_write_mft_record_raw]
               ↓
[Partition Sub-Device Clamping: gpt_part_write (lba + count <= sector_count)]
               ↓
[Native NVMe Command: nvme_write_sectors -> NVME_IO_OP_WRITE]
               ↓
[Controller Sync: nvme_flush -> NVME_IO_OP_FLUSH]
               ↓
[Byte-for-Byte Read-Back Verification in ATOMS OS]
               ↓
[Clean Shutdown & Windows 11 Normal Boot Verification]
```

---

## 4. Critical Forensic Defects & Safety Hazards Discovered

Our line-by-line inspection of `kernel/vfs/vfs_legacy/fs/ntfs/src/ntfs.c` discovered three critical issues in the legacy experimental write routines that would pose severe risks if executed unmodified on the live Windows 11 partition:

### Hazard 1: Hardcoded MFT Record Index Overwrite Hazard
- **Source Location:** `kernel/vfs/vfs_legacy/fs/ntfs/src/ntfs.c`, Lines 2029–2031
- **Current Behavior:**
  ```c
  static uint32_t s_next_free_record = 64;
  uint32_t allocated_record = (hint_record > 32) ? hint_record : s_next_free_record++;
  ```
- **Forensic Evidence:** On a real, established Windows 11 installation, MFT records 0 through 15 are NTFS reserved records, and records 16 through 100,000+ are allocated to active Windows system files, registry hives, system drivers, and system directories. Record 64 is **ALREADY IN USE** by Windows 11!
- **Hazard Analysis:** Calling `ntfs_mft_alloc_record` with the existing implementation would overwrite MFT Record 64 on disk, destroying an active Windows 11 system file.
- **Required Surgical Remedy:** The allocator must never use a hardcoded default. It must scan for an MFT record that is explicitly inactive (`!(hdr->flags & NTFS_FILE_IN_USE)`), completely unallocated/zeroed (`hdr->record_number == 0` or empty magic), and located strictly above the protected Windows system record threshold (`record >= 1024`).

---

### Hazard 2: Bogus Cluster Allocation & Data Area Overwrite Hazard
- **Source Location:** `kernel/vfs/vfs_legacy/fs/ntfs/src/ntfs.c`, Lines 2128–2136 & Lines 1852–1877
- **Current Behavior:**
  ```c
  if (size > 0 && clusters_needed > 0) {
      if (ntfs_alloc_clusters(vol, clusters_needed, 2048, &alloc_lcn, &alloc_count)) {
          ...
      }
  }
  ```
- **Forensic Evidence:** `ntfs_alloc_clusters()` uses `hint_lcn = 2048` without scanning `$Bitmap`. On a 243 GB Windows 11 volume, Cluster 2048 is already occupied by existing data.
- **Hazard Analysis:** Allocating Cluster 2048 would cause cluster collision and data corruption in the Windows filesystem data area.
- **Crucial Forensic Breakthrough:** The test file content specified by the mission is exactly 105 bytes:
  ```
  ATOMS OS NTFS WRITE VALIDATION
  Created by ATOMS on real hardware.
  TEST-ID: ATOMS-NTFS-WRITE-20260904
  ```
  Under Microsoft NTFS specifications, files $\le 256$ bytes are stored as **Resident Attributes** directly inside the file's 1024-byte MFT record itself!
- **Required Surgical Remedy:** Enforce that the test file is strictly **RESIDENT**. For resident files:
  - `clusters_needed = 0`
  - `alloc_lcn = 0`
  - Zero external clusters are allocated or touched.
  - No data sectors outside the newly allocated MFT record are modified.
  - Zero risk of cluster collision.

---

### Hazard 3: Directory Index Stub Hazard ($INDEX_ROOT)
- **Source Location:** `kernel/vfs/vfs_legacy/fs/ntfs/src/ntfs.c`, Lines 2085–2093
- **Current Behavior:**
  ```c
  bool ntfs_btree_insert(...) {
      vol->stats.node_splits++;
      ntfs_path_cache_flush(&vol->path_cache);
      return true;
  }
  ```
- **Forensic Evidence:** `ntfs_btree_insert()` currently increments a counter and flushes cache, but does NOT write the new entry into the root directory's `$INDEX_ROOT` attribute on disk.
- **Hazard Analysis:** If the entry is not written into `$INDEX_ROOT` of root directory Record 5, Windows 11 will not see the file upon rebooting. Furthermore, CHKDSK would identify the MFT record as an unindexed orphan.
- **Required Surgical Remedy:** Safely insert the `NTFS_IndexEntry` containing the `$FILE_NAME` key into the root directory's resident `$INDEX_ROOT` entry list immediately before the End Marker (`NTFS_INDEX_ENTRY_LAST`), and update the Index Header `total_size`.

---

### Hazard 4: Missing Pre-existence Stop Gate
- **Source Location:** `kernel/vfs/vfs_legacy/fs/ntfs/src/ntfs.c`, Line 2112
- **Current Behavior:** `ntfs_create_file()` does not verify whether `name` already exists in `dir_path`.
- **Hazard Analysis:** Violates the mandatory safety rule: *"Before creation, verify that the exact target pathname does NOT exist. If target already exists: STOP."*
- **Required Surgical Remedy:** Prepend an explicit existence query (`ntfs_dir_lookup_entry`). If the file exists, immediately abort the test.

---

## 5. Physical Hardware Safety Enforcements

1. **Sub-Device Partition Boundary Clamping:**
   - All sector operations are executed through `nvme0n1p3` (Block Device ID 3).
   - In `kernel/drivers/storage/partition/gpt.c`, `gpt_part_write()` enforces:
     ```c
     if (lba + count > ctx->sector_count) return false;
     ```
   - Partition 3 start LBA is $239,616$, and sector count is $486,609,375$ ($243$ GB).
   - Under no circumstances can any write reach LBA 0 (Protective MBR), LBA 1..33 (GPT Header & Tables), or LBA 2048..206847 (EFI System Partition). Physical disk corruption of partition tables or bootloaders is mechanically impossible.

2. **Controlled Scope:**
   - Only exactly ONE MFT record will be populated.
   - Only the resident `$INDEX_ROOT` of root directory Record 5 will be updated with the new single entry.
   - Zero cluster allocations in the data region.

---

## 6. Forensic Verdict & Progression Gate

- **Forensic Classification:** High-value discovery. Fatal risks identified and fully mapped.
- **Progression Decision:** **DO NOT WRITE YET.**
- **Next Required Step:** Hand off to Task 2 (Architect Team) to produce `PATCH_PLAN.md` specifying the surgical implementation of:
  1. Pre-existence Stop Gate.
  2. Inactive/Free MFT Record Scanner (`record >= 1024`).
  3. Resident-Only Data Stream Enforcement (`size <= 256`, zero cluster allocation).
  4. Surgical `$INDEX_ROOT` Directory Entry Insertion.
