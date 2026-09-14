# ATOMS OS — NTFS Implementation Gap Analysis
**Target:** Current `kernel/vfs/vfs_legacy/fs/ntfs/src/ntfs.c` vs Microsoft NTFS v3.1 Specification  
**Classification:** READ-ONLY ARCHITECTURAL AUDIT — NO FIXES APPLIED  

---

## Gap Analysis by Operation

---

### 1. CREATE FILE
- **SPECIFICATION:**
  Must find a free MFT record via Record 0 `$BITMAP`, set bit in `$BITMAP`, construct base record with ordered attributes (`0x10`, `0x30`, `0x80`, `0xFFFFFFFF`), insert into parent directory B-tree respecting collation order and node tiering, and flush.
- **CURRENT ATOMS CODE:**
  `ntfs_create_file()` ([ntfs.c:2338-2500](file:///d:/Signatures_OS/kernel/vfs/vfs_legacy/fs/ntfs/src/ntfs.c#L2338-L2500)). Calls `ntfs_mft_alloc_record()`, builds record buffer, calls `ntfs_write_mft_record_raw()`, then calls `ntfs_btree_insert()`.
- **ACTUAL DISK OPERATION:**
  Wrote Record 2766 to Extent 1 LBA. Appended index entry to Record 5. **Omitted write to Record 0 $BITMAP.**
- **MISSING REQUIREMENT:**
  1. Setting and committing bit in Record 0's `$BITMAP`.
  2. B-Tree collation-order insertion rather than appending to `$INDEX_ROOT`.
- **RISK:**
  Windows BugCheck `0x24` due to bitmap-to-record inconsistency and index collation violation.
- **TEST REQUIRED:**
  Full multi-extent allocation and binary-sorted insertion test.

---

### 2. DELETE FILE
- **SPECIFICATION:**
  Remove entry from parent directory index, rebalance B-tree. If hard links reach zero, free cluster runs in `$Bitmap`, clear `IN_USE` in record header, clear bit in Record 0 `$BITMAP`, increment sequence number.
- **CURRENT ATOMS CODE:**
  `ntfs_delete_node()` ([ntfs.c:2655-2715](file:///d:/Signatures_OS/kernel/vfs/vfs_legacy/fs/ntfs/src/ntfs.c#L2655-L2715)). Calls `ntfs_btree_delete()` and `ntfs_mft_free_record_num()`.
- **ACTUAL DISK OPERATION:**
  Only marks record buffer in memory; does not balance B-tree or update `$BITMAP` on disk.
- **MISSING REQUIREMENT:**
  B-Tree underflow merge/rebalance, cluster bitmap deallocation, Record 0 bitmap synchronization.
- **RISK:**
  Orphaned index references and leaked disk clusters.
- **TEST REQUIRED:**
  Directory deletion and cluster reclaim verification test.

---

### 3. RENAME
- **SPECIFICATION:**
  Remove old `$FILE_NAME` entry from source directory B-tree. Rebalance source directory. Insert new `$FILE_NAME` entry into target directory B-tree in collation order. Update `$FILE_NAME` attribute in target file record.
- **CURRENT ATOMS CODE:**
  `ntfs_rename_node()` ([ntfs.c:2720-2775](file:///d:/Signatures_OS/kernel/vfs/vfs_legacy/fs/ntfs/src/ntfs.c#L2720-L2775)).
- **ACTUAL DISK OPERATION:**
  Modifies in-memory record, calls dummy B-tree delete and insert.
- **MISSING REQUIREMENT:**
  Cross-node and cross-tier B-tree key relocation; `$FILE_NAME` attribute rewrite.
- **RISK:**
  Duplicate index entries or missing directory references.
- **TEST REQUIRED:**
  Rename across small and large directory boundaries.

---

### 4. WRITE (NON-RESIDENT)
- **SPECIFICATION:**
  If write extends past resident threshold (256–700B), convert attribute to non-resident. Allocate clusters via `$Bitmap`, construct runlist, update attribute header (`data_size`, `allocated_size`, `initialized_size`), and update directory `$FILE_NAME` size cache.
- **CURRENT ATOMS CODE:**
  `ntfs_alloc_clusters()` ([ntfs.c:1920-2050](file:///d:/Signatures_OS/kernel/vfs/vfs_legacy/fs/ntfs/src/ntfs.c#L1920-L2050)).
- **ACTUAL DISK OPERATION:**
  Only handles resident payloads for controlled test; non-resident write path was bypassed.
- **MISSING REQUIREMENT:**
  Runlist expansion, cluster bitmap persistence, directory file size synchronization.
- **RISK:**
  Filesystem truncation or overwriting neighboring cluster runs.
- **TEST REQUIRED:**
  Cluster allocation boundary and fragmented runlist decode/encode test.

---

### 5. TRUNCATE
- **SPECIFICATION:**
  Free clusters past new EOF in `$Bitmap`. Truncate or consolidate runlists in non-resident `$DATA`. Update `data_size` and `initialized_size`. If new size fits in record, convert back to resident.
- **CURRENT ATOMS CODE:**
  No dedicated truncate implementation in `ntfs.c`.
- **ACTUAL DISK OPERATION:**
  Unsupported.
- **MISSING REQUIREMENT:**
  Cluster deallocation and resident conversion pipeline.
- **RISK:**
  Stale cluster pointers pointing to freed blocks.
- **TEST REQUIRED:**
  Truncation from non-resident to resident.

---

### 6. DIRECTORY INSERT
- **SPECIFICATION:**
  Search parent directory B-tree using collation rules. If key fits in leaf node, insert in sorted order. If leaf overflows, split node, promote median key to parent node, and allocate child INDX block via directory `$BITMAP`.
- **CURRENT ATOMS CODE:**
  `ntfs_btree_insert()` ([ntfs.c:2212-2319](file:///d:/Signatures_OS/kernel/vfs/vfs_legacy/fs/ntfs/src/ntfs.c#L2212-L2319)).
- **ACTUAL DISK OPERATION:**
  Found End Marker in Record 5 `$INDEX_ROOT`, shifted memory, appended new entry immediately before End Marker.
- **MISSING REQUIREMENT:**
  1. Collation-based binary search positioning.
  2. B-Tree node splitting and index allocation expansion.
  3. Proper sub-node pointer routing when `$INDEX_ALLOCATION` exists.
- **RISK:**
  🔴 **PRIMARY ROOT CAUSE OF BSOD 0x24**: Breaks B-tree search invariants and directory sorting.
- **TEST REQUIRED:**
  B-Tree collation sort test and multi-tier INDX insertion test.

---

### 7. DIRECTORY DELETE
- **SPECIFICATION:**
  Locate entry via collation binary search. Remove entry. If node falls below minimum entries, merge with sibling or borrow from sibling. If root node shrinks, collapse tree.
- **CURRENT ATOMS CODE:**
  `ntfs_btree_delete()` ([ntfs.c:2321-2328](file:///d:/Signatures_OS/kernel/vfs/vfs_legacy/fs/ntfs/src/ntfs.c#L2321-L2328)). Only flushes path cache; performs no disk mutation.
- **ACTUAL DISK OPERATION:**
  No-op.
- **MISSING REQUIREMENT:**
  Index entry extraction and node compaction.
- **RISK:**
  Phantom index entries pointing to stale MFT records.
- **TEST REQUIRED:**
  Leaf and intermediate index entry deletion test.

---

### 8. MFT ALLOCATION
- **SPECIFICATION:**
  Query Record 0 `$BITMAP` using `vol->mft_extent_map`. Find bit $R == 0$. Set bit $R = 1$ in bitmap buffer. Write updated `$BITMAP` sector to disk. Initialize Record $R$ with `'FILE'` magic, sequence number, and valid USA trailers.
- **CURRENT ATOMS CODE:**
  `ntfs_mft_alloc_record()` ([ntfs.c:2122-2202](file:///d:/Signatures_OS/kernel/vfs/vfs_legacy/fs/ntfs/src/ntfs.c#L2122-L2202)).
- **ACTUAL DISK OPERATION:**
  Queried Record 0 `$BITMAP` and found 2766 free. Probed raw LBA without extent mapping. Omitted write to Record 0 `$BITMAP`.
- **MISSING REQUIREMENT:**
  1. Extent mapping during physical candidate verification.
  2. Setting and writing bit $R = 1$ to Record 0's `$BITMAP`.
- **RISK:**
  🔴 **CONFIRMED BSOD 0x24 TRIGGER**: Inconsistency between MFT record `IN_USE` flag and Record 0 `$BITMAP`.
- **TEST REQUIRED:**
  Dual-extent MFT allocation and bitmap synchronization test.

---

### 9. MFT FREE
- **SPECIFICATION:**
  Clear `IN_USE` flag in record header, increment `sequence_number`, clear bit $R$ in Record 0 `$BITMAP`.
- **CURRENT ATOMS CODE:**
  `ntfs_mft_free_record_num()` ([ntfs.c:2205-2240](file:///d:/Signatures_OS/kernel/vfs/vfs_legacy/fs/ntfs/src/ntfs.c#L2205-L2240)).
- **ACTUAL DISK OPERATION:**
  Only sets in-memory flags; does not write to disk.
- **MISSING REQUIREMENT:**
  Bitmap bit clear and disk persistence.
- **RISK:**
  Leaked MFT records that are never reclaimed.
- **TEST REQUIRED:**
  MFT free and reallocation sequence verification.

---

### 10. BITMAP UPDATE
- **SPECIFICATION:**
  Read cluster containing target bit, flip bit, update sector trailer USA fixups if applicable, write sector back to storage, and flush.
- **CURRENT ATOMS CODE:**
  Read-only bitmap query implemented (`ntfs_mft_bitmap_is_record_free`); write path omitted.
- **ACTUAL DISK OPERATION:**
  Zero bitmap updates committed.
- **MISSING REQUIREMENT:**
  Atomic bit mutation and sector writeback.
- **RISK:**
  Filesystem allocation metadata drift.
- **TEST REQUIRED:**
  Bitmap bit set/clear verification test.

---

### 11. ATTRIBUTE UPDATE
- **SPECIFICATION:**
  Ensure attribute list remains sorted by attribute type code (`0x10 < 0x20 < 0x30 < 0x80 < 0xFFFFFFFF`). Ensure all attribute lengths are multiples of 8 bytes. Ensure `bytes_in_use` and `bytes_allocated` match.
- **CURRENT ATOMS CODE:**
  Constructs `$STANDARD_INFORMATION`, `$FILE_NAME`, and `$DATA` sequentially.
- **ACTUAL DISK OPERATION:**
  Built valid attributes for Record 2766, but lacked dynamic attribute list resizing for large records.
- **MISSING REQUIREMENT:**
  Dynamic attribute insertion and `$ATTRIBUTE_LIST` spillover handling.
- **RISK:**
  Buffer overflow if attributes exceed 1024 bytes.
- **TEST REQUIRED:**
  Attribute insertion with 8-byte alignment verification.

---

### 12. INDEX UPDATE
- **SPECIFICATION:**
  Binary search index node. If entry exists, update file size and timestamp cache. If new, insert maintaining `$UpCase` collation.
- **CURRENT ATOMS CODE:**
  Linear append before End Marker.
- **ACTUAL DISK OPERATION:**
  Violated collation sort order.
- **MISSING REQUIREMENT:**
  Collation binary search and proper index placement.
- **RISK:**
  🔴 **CONFIRMED BSOD 0x24 TRIGGER**.
- **TEST REQUIRED:**
  Index collation sorting verification test.

---

### 13. FLUSH
- **SPECIFICATION:**
  Issue hardware cache sync command to NVMe/SATA controller. Ensure volatile buffers are committed.
- **CURRENT ATOMS CODE:**
  `nvme_flush(1)` via `kernel/drivers/storage/nvme/nvme.c`.
- **ACTUAL DISK OPERATION:**
  Executed NVMe FLUSH command on NSID 1 with CQE status `0x0000` (Success).
- **MISSING REQUIREMENT:**
  None at hardware layer. Hardware flush executed cleanly.
- **RISK:**
  None at hardware level.
- **TEST REQUIRED:**
  Cache synchronization timing test.
