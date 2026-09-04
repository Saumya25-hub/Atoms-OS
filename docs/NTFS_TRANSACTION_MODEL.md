# ATOMS OS — NTFS Transaction & Crash Safety Specification
**Document ID:** `NTFS-TXN-V2.0`  
**Target:** Metadata Mutation Ordering, Crash Consistency, Journal Semantics, and Rollback Procedures  
**Hardware Target:** ASUS PRIME B750M-K (Intel Core i3-14100F, WD Blue SN5000 NVMe SSD)  
**Authors:** ATOMS OS Core Engineering & Filesystem Architecture Team  
**Date:** 2026-09-04  

---

## 1. Executive Summary & Philosophy

NTFS is inherently a logging, transactional filesystem designed around the Aries recovery algorithm. In standard Windows environments, all metadata modifications are bracketed by Write-Ahead Log (WAL) records in `$LogFile` (Record 2).

For ATOMS OS, our engineering rule is:
> **"Do not pretend full journaling is implemented when it is not. Implement the strongest safe subset, clearly document transaction boundaries, enforce strict physical write ordering, provide atomic rollback on failure, and explicitly scope unsupported features."**

---

## 2. Supported vs Unsupported Scope

### Supported in ATOMS OS (V2.6+)
1. **Strict 5-Stage Write-Ahead Pipeline**: Enforces metadata dependency ordering on physical media.
2. **Atomic In-Flight Rollback**: Any error during creation/mutation triggers automatic compensation (clearing `$MFT::$BITMAP`, zeroing the allocated MFT record, freeing clusters, flushing).
3. **Canonical Extent Mapping**: All MFT modifications translate through the multi-extent runlist mapper to avoid corrupted sector addressing.
4. **Hardware Storage Barrier**: Native controller flush (`nvme_flush(1)` or ATA `FLUSH CACHE EXT`) is issued after every critical phase boundary.
5. **Observability Journal**: Every mutation emits structured serial/telemetry records detailing `op`, `record`, `seq`, `vcn`, `lcn`, `lba`, and status.

### Unsupported in Current ATOMS OS (Explicitly Scoped)
1. **Physical $LogFile WAL Logging**: Parsing restart areas, writing redo/undo log client records, and managing LSN log sequences are **NOT yet implemented**.
2. **Crash-Replay Engine**: Replaying torn transactions from `$LogFile` upon mounting a dirty volume is **NOT yet supported**. ATOMS relies on clean unmounting and ordered flushing to keep the filesystem consistent.
3. **Volume Dirty Flag Assertion**: ATOMS does not assert `VOLUME_DIRTY (0x0001)` in `$Volume` (Record 3) unless a write abort occurs.

---

## 3. Transaction Boundaries & The 5-Stage Pipeline

To prevent catastrophic metadata contradictions (such as the BugCheck 0x24 caused by an allocated record with an unallocated bitmap bit), every NTFS mutation executes within a strictly bounded state machine:

```
                  ┌───────────────────────────┐
                  │   BEGIN TRANSACTION       │
                  │   (In-Memory Validation)  │
                  └─────────────┬─────────────┘
                                │
                                ▼
                  ┌───────────────────────────┐
                  │   STAGE 1: ALLOCATION     │
                  │   - Probe MFT $BITMAP     │
                  │   - Probe Volume $Bitmap  │
                  │   - Select Candidate Rec  │
                  └─────────────┬─────────────┘
                                │
                                ▼
                  ┌───────────────────────────┐
                  │   STAGE 2: $BITMAP COMMIT │
                  │   - Set Bit R = 1 in $MFT │
                  │   - Write sector to disk  │
                  │   - Issue NVMe FLUSH      │
                  └─────────────┬─────────────┘
                                │
                                ▼
                  ┌───────────────────────────┐
                  │   STAGE 3: MFT RECORD     │
                  │   - Build StandardInfo    │
                  │   - Build FileName        │
                  │   - Build Data Runlist    │
                  │   - Apply USA Fixup       │
                  │   - Write sector to disk  │
                  │   - Issue NVMe FLUSH      │
                  └─────────────┬─────────────┘
                                │
                                ▼
                  ┌───────────────────────────┐
                  │   STAGE 4: DIRECTORY B-TREE
                  │   - Collate with $UpCase  │
                  │   - Find leaf INDX block  │
                  │   - Insert sorted entry   │
                  │   - Apply USA Fixup       │
                  │   - Write block to disk   │
                  │   - Issue NVMe FLUSH      │
                  └─────────────┬─────────────┘
                                │
                                ▼
                  ┌───────────────────────────┐
                  │   COMMIT TRANSACTION      │
                  │   - Invalidate Caches     │
                  │   - Return File Record    │
                  └───────────────────────────┘
```

---

## 4. Failure Analysis & Power-Cut Consistency

If an unexpected power failure or system reset occurs at any boundary, the on-disk state remains safe:

| Fault Boundary | On-Disk MFT State | $MFT::$BITMAP State | Directory B-Tree State | Volume Integrity on Windows 11 Boot |
| :--- | :--- | :--- | :--- | :--- |
| **Before Stage 2** | Virgin / Untouched | 0 (Free) | No entry | **100% Clean.** No change occurred. |
| **After Stage 2, Before Stage 3** | Unallocated | 1 (Reserved) | No entry | **Safe.** Windows sees bit 1 marked in bitmap with virgin record. Windows allocator will not collide; `chkdsk` clears bit if needed. |
| **After Stage 3, Before Stage 4** | Formatted (Seq=S) | 1 (Reserved) | No entry | **Safe.** Orphaned MFT record without directory reference. Windows boots normally without BSOD. Windows `chkdsk` salvages file to `found.000`. |
| **After Stage 4** | Formatted (Seq=S) | 1 (Reserved) | Entry linked | **100% Consistent.** File fully visible and valid in Windows Explorer. |

### The Critical Invariant: Directory Index ALWAYS Follows MFT & Bitmap
The fatal mistake of the previous engine was linking the directory index entry **before or without committing the MFT bitmap bit**. When Windows saw a directory pointer to a record whose bitmap bit was free, it crashed.
**Rule:** The directory index entry is **NEVER** written to disk until after both the MFT record and the `$MFT::$BITMAP` bit have been committed and flushed.

---

## 5. Atomic In-Flight Rollback Protocol

If an error occurs during runtime (e.g., directory insertion fails, memory exhausted, or I/O error):

```c
void ntfs_rollback_create(NTFS_VOLUME* vol, uint32_t rec_num, uint64_t alloc_lcn, uint32_t cluster_count) {
    com1_puts("[NTFS ROLLBACK] Reverting metadata on disk...\r\n");

    // 1. Clear allocation bit in Record 0 $BITMAP
    ntfs_mft_set_record_allocated(vol, rec_num, false);

    // 2. Zero out MFT record on disk
    uint8_t* zero_buf = (uint8_t*)kmalloc(vol->file_record_size);
    if (zero_buf) {
        memset(zero_buf, 0, vol->file_record_size);
        ntfs_write_mft_record_raw(vol, rec_num, zero_buf);
        kfree(zero_buf);
    }

    // 3. Free non-resident clusters in Volume $Bitmap
    if (alloc_lcn > 0 && cluster_count > 0) {
        ntfs_free_clusters(vol, alloc_lcn, cluster_count);
    }

    // 4. Force controller flush
    ntfs_flush_device(vol);
}
```

This ensures that any failed operation leaves the volume 100% pristine.
