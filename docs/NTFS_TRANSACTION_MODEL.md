# ATOMS OS — NTFS Transaction & Crash Safety Specification
**Document ID:** `NTFS-TXN-V1.0`  
**Target:** Metadata Mutation Ordering, Crash Consistency, Journal Semantics, and Rollback Procedures  
**Hardware Target:** ASUS PRIME B750M-K (Intel Core i3-14100F, WD Blue SN5000 NVMe SSD)  
**Authors:** Developer A (Architecture) & Developer B (Forensics)  
**Date:** 2026-09-04  

---

## 1. Transactional Principles in NTFS

NTFS is a journaling, transaction-oriented filesystem. In native Windows implementations, structural metadata changes are protected via the `$LogFile` redo/undo transaction journal.
In the ATOMS OS Native NTFS Engine, write operations must preserve **crash consistency** so that an unexpected power loss or reboot at any intermediate micro-step leaves the physical volume in a recoverable state without corrupting user data or triggering `BugCheck 0x24`.

---

## 2. The Strict 5-Stage Write-Ahead Pipeline

Every file creation, deletion, or modification must execute through an explicit 5-stage pipeline:

```
[STAGE 1: PRE-ALLOCATION & RESERVATION]
  Verify filename does not already exist in directory B-tree.
  Locate candidate free MFT record R from Record 0 $BITMAP.
  Locate free clusters from Volume $Bitmap (if non-resident).
  Hold reservations in volatile memory.

[STAGE 2: PAYLOAD & RECORD WRITE]
  Write external data clusters to disk (if non-resident).
  Format candidate MFT record buffer with attributes and USA fixups.
  Write MFT record R to physical LBA.
  Barrier: Issue NVMe FLUSH command.

[STAGE 3: BITMAP ALLOCATION COMMIT]
  Set bit R = 1 in Record 0's $BITMAP.
  Write modified $BITMAP sector to physical LBA.
  Barrier: Issue NVMe FLUSH command.

[STAGE 4: DIRECTORY B-TREE INSERTION]
  Insert $FILE_NAME key into parent directory B-Tree in sorted collation position.
  Write updated directory index blocks/records to physical LBA.
  Barrier: Issue NVMe FLUSH command.

[STAGE 5: TRANSACTION COMMIT & CLEAN UNMOUNT]
  Update volume performance statistics.
  Clear transient in-memory reservations.
  If volume is cleanly unmounted, ensure volume dirty flag remains 0.
```

---

## 3. Failure Analysis at Every Pipeline Boundary

If a power failure or fault occurs at any stage, the volume state behaves predictably:

| Fault Point | MFT Record State | $MFT::$BITMAP State | Parent Directory State | Result on Windows Reboot |
| :--- | :--- | :--- | :--- | :--- |
| **During Stage 1** | Unchanged | Free (0) | Unchanged | Clean. Zero effect. |
| **After Stage 2** | Record written | Free (0) | Unchanged | Clean. Directory has no reference. Windows may overwrite slot. Zero corruption. |
| **After Stage 3** | Record written | Allocated (1) | Unchanged | Orphaned file record (no directory reference). Windows boots cleanly; `chkdsk` can optionally salvage into `found.000`. |
| **After Stage 4** | Record written | Allocated (1) | Entry linked | **100% Consistent.** File fully visible and valid in Windows. |

### Why the Previous Test Failed
In the earlier test (Commit `90d1718`), the engine executed **Stage 2** and **Stage 4**, but **completely skipped Stage 3**!
This left:
- Record 2766 = `IN_USE` (Stage 2 committed).
- Directory Entry = Linked (Stage 4 committed).
- `$MFT::$BITMAP` = `FREE` (Stage 3 omitted!).
This inverted order created an impossible state that Windows `ntfs.sys` rejected.

---

## 4. Rollback Engine Specifications

If any operation fails during runtime prior to Stage 4 completion:
1. **Rollback Directory:** If directory entry was modified, shift entries back and restore original length.
2. **Rollback Bitmap:** Clear bit $R$ to `0` in Record 0 `$BITMAP` and write back to disk.
3. **Rollback Record:** Clear `flags` to `0x0000` in record $R$ and write back to disk.
4. **Flush:** Issue hardware cache barrier.
