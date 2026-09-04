# ATOMS OS — Architecture Patch Plan (Task 2)
## Subject: Option C — Surgical Rollback of Record 5 Root Directory Index Entry
**Input Document:** `FORENSIC_REPORT.md`, `docs/ntfs/POST_BSOD_FORENSIC_AUTOPSY.md`  
**Target Hardware:** ASUS PRIME B750M-K | Intel Core i3-14100F | WD Blue SN5000 500GB NVMe SSD  
**Target Volume:** Partition 3 (Start LBA: 239616, Size: 243 GB, Physical Windows 11 NTFS)  
**Date:** 2026-09-04  
**Author:** Task 2 — Architect Team  
**Git Checkpoint Commit:** `1b6da22` (`feat(debug): implement deep read-only NTFS forensic inspection screen`)

---

## 1. Objective & Architectural Rationale

On-disk forensic telemetry from the physical hardware (`forensic_screen_20260904_183437_s1.png`) proved that:
1. Physical MFT Record 5 (Root Directory) contains an out-of-order 120-byte index entry (`ATOMS_WRITE_TEST.txt`) in `$INDEX_ROOT`.
2. This entry violates B-Tree collation order (`'ATOMS_WRITE_TEST.txt'` placed after `'ATOMS_WRI~'`) and has leaf flags (`0x0000`) inside a router node.
3. When Windows 11 boots, `ntfs.sys` detects this collation violation in `NtfsFindIndexEntry()` and halts the system with `STOP CODE: NTFS_FILE_SYSTEM (0x24)`.
4. Removing this single 120-byte orphaned entry from Record 5 restores the root directory index to its original, valid Windows 11 B-tree state, allowing Windows 11 to boot cleanly with zero data loss.

---

## 2. Files and Functions Scheduled for Modification

| Action | File Path | Function / Subsystem | Modification Scope |
| :---: | :--- | :--- | :--- |
| **MODIFY** | `kernel/debug/storage_forensic_debug.c` | `storage_forensic_debug_run()` | Add controlled Option C surgical rollback execution routine guarded by compile-time / runtime gate. |
| **MODIFY** | `kernel/vfs/vfs_legacy/fs/ntfs/src/ntfs.c` | `ntfs_btree_remove_entry()` | Implement surgical in-memory 120-byte shift and header decrement for Record 5. |

*Absolute Invariant: No changes to UEFI bootloader, PMM, VMM, ACPI, USB HID, or partition table routines.*

---

## 3. Surgical Rollback Manifest & Byte-Level Specifications

### Target: MFT Record 5 (Physical LBA `6291466` on Partition 3)
- **Sector Count:** 2 sectors (1024 bytes).
- **Current On-Disk State:**
  - `idx_hdr->total_size`: 640 bytes (includes 120-byte test entry).
  - `idx_hdr->allocated_size`: 640 bytes.
  - `res_hdr->value_length`: 656 bytes.
  - `attr_hdr->length`: 688 bytes.
  - `fhdr->bytes_in_use`: 696 bytes.
  - Index contains 120-byte `ATOMS_WRITE_TEST.txt` entry at offset `P`.
- **Target Restored State:**
  - Shift memory from offset `P + 120` to `fhdr->bytes_in_use` backward by 120 bytes.
  - Zero out the trailing 120 bytes in the buffer.
  - Decrement all 5 size fields by exactly 120:
    - `idx_hdr->total_size -= 120` (Restored to 520)
    - `idx_hdr->allocated_size -= 120` (Restored to 520)
    - `res_hdr->value_length -= 120` (Restored to 536)
    - `attr_hdr->length -= 120` (Restored to 568)
    - `fhdr->bytes_in_use -= 120` (Restored to 576)
  - Increment USA sequence number: `usa[0]++`.
  - Back up real sector-end words into `usa[1]` and `usa[2]`, then overwrite offsets 510 and 1022 with `usa[0]`.
  - Write 2 sectors to Physical LBA `6291466`.
  - Issue native `NVMe FLUSH` command to Namespace 1.

---

## 4. Safety & Non-Destructive Guarantees

1. **Zero User Data Touched:** No clusters across the 243 GB data partition are touched or modified.
2. **Zero In-Use Windows Files Modified:** Record 5 is the directory index; removing the test entry affects ONLY the non-existent test file.
3. **Partition Table Unreachable:** LBA bounds are strictly clamped to Partition 3.

---

## 5. Rollback Plan

Before writing to disk:
1. The exact original 1024-byte Record 5 sector buffer is saved in memory.
2. If any write verification fails, the original 1024-byte buffer is immediately rewritten.

---

## 6. Verification & Certification Plan

1. **Pre-Flight:** Clean compilation with `build.ps1`, QEMU verification with `tools/test_qemu_nvme.ps1`.
2. **PXE Hardware Boot:** Boot physical ASUS B750M-K via PXE.
3. **Hardware Telemetry:** Capture new live 1080p screenshot via UDP 9998:
   - Verify Record 5 shows `ATOMS_WRITE_TEST.txt [REMOVED]`.
   - Verify Record 5 directory index collation is valid.
   - Verify zero errors and active heartbeat spinner.
4. **Reboot to Windows 11:** Power cycle PC and verify Windows 11 boots straight to desktop without BSOD 0x24.
