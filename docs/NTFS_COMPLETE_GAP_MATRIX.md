# ATOMS OS — Complete NTFS Gap Matrix & Compatibility Audit
**Document ID:** `NTFS-GAP-MATRIX-V1.0`  
**Target Architecture:** Native ATOMS OS Read/Write NTFS Engine  
**Hardware Target:** ASUS PRIME B750M-K (Intel Core i3-14100F, WD Blue SN5000 NVMe 500GB SSD)  
**Reference Implementations:** Microsoft NTFS 3.1 (`ntfs.sys`), Linux Kernel 6.x (`fs/ntfs3`), NTFS-3G  
**Classification:** STRICT COMPARATIVE AUDIT  
**Date:** 2026-09-04  

---

## 1. Executive Gap Matrix Summary

| Subsystem / Feature | ATOMS Current State | NTFS Format Spec | Linux ntfs3 Behavior | Windows Compatibility | Status |
| :--- | :--- | :--- | :--- | :--- | :---: |
| **Boot Sector (BPB)** | Full BPB parse, OEM ID check, sector/cluster calculation | 512B VBR, jump opcode, 0xAA55 sig, backup boot sector | Checks primary + backup VBR, verifies volume dirty flag | Validates geometry strictly during winload.efi | 🟢 GREEN |
| **MFT Discovery** | Record 0 read, primary $DATA runlist decoded | Record 0 describes entire MFT | Maps MFT via runlists in memory | Reads Record 0 at boot | 🟢 GREEN |
| **MFT Mirror ($MFTMirr)**| Reads mirror if primary fails; does not update mirror on write | Duplicate of records 0-3 at mirror LCN | Keeps mirror in sync on every write to records 0-3 | Checks mirror consistency | 🟡 YELLOW |
| **MFT Extent Mapping** | Canonical `ntfs_mft_record_to_physical_lba()` translates VCN to LCN | Multi-extent non-contiguous MFT | Full dynamic runlist lookup (`ntfs_bmap`) | Required for multi-extent MFT | 🟢 GREEN |
| **MFT Allocation ($BITMAP)**| Reads Record 0 `$BITMAP` to find free bit; **NEVER sets bit or flushes to disk** | Bit $N=1$ in Record 0 `$BITMAP` denotes allocated record $N$ | `ntfs_look_free_mft` sets bit, marks buffer dirty, flushes | Mismatch between record flag and bitmap bit triggers BSOD 0x24 | 🔴 RED |
| **File Record Header** | Validates/creates 48B header (`FILE`, USA, offsets, flags, used/alloc) | Standard 48B header + USA, 8-byte aligned attributes | Strict validation of header bounds and alignments | Strict validation in `ntfs.sys` | 🟢 GREEN |
| **USA / Fixup Protection** | Implemented for MFT records (`apply_fixup`, `write_raw`); missing for INDX | Multi-sector transfer protection (trailer replacement) | Checked and applied on all MFT and INDX buffers | Bad fixup triggers corrupted record panic | 🟡 YELLOW |
| **Sequence Numbers** | Hardcoded `seq = 1` on new record; ignores prior on-disk sequence value | Incremented on every reallocation; never 0 in active file | `seq = le16_to_cpu(hdr->seq) + 1` upon slot reuse | Stale/incorrect sequence breaks file references | 🔴 RED |
| **Attribute Ordering** | Inserts attributes in order: $10, $30, $80, $FF | Attributes MUST be strictly sorted by Type Code ($10 < $20 < $30...) | Enforces ascending numerical order by type code | Unsorted attributes cause instant BSOD 0x24 | 🟢 GREEN |
| **$STANDARD_INFORMATION** | 48-byte resident header with 4 timestamps and DOS flags | 48B (NTFS 1.2) or 72B (NTFS 3.0+ with Security ID & Quota) | Writes 72B standard info attribute with owner/security IDs | Reads 72B attributes on Win 10/11 | 🟡 YELLOW |
| **$FILE_NAME** | Resident attribute with parent ref, timestamps, sizes, UTF-16LE name | UTF-16LE characters, namespaces (0=POSIX, 1=Win32, 2=DOS, 3=Both) | Supports dual names (Win32 + 8.3 DOS short name) | Requires 8.3 alias for legacy/short names | 🟡 YELLOW |
| **$DATA (Resident)** | Fully supported for files $\le 256$ bytes; zero cluster allocation | Payload stored in MFT record when size fits in record slack | Converts between resident and non-resident on threshold | Fully supported | 🟢 GREEN |
| **$DATA (Non-Resident)** | Decodes runlists; allocates clusters via `$Bitmap`, but write extension incomplete | Cluster allocation outside MFT mapped via compressed data runs | Full dynamic cluster allocation, truncation, and hole punching | Fully supported | 🟡 YELLOW |
| **Data Run Encoding** | `ntfs_encode_data_runs` compresses LCN diffs and cluster counts | Variable-length nibble-encoded relative LCN offsets | Full runlist compression and defragmentation | Validates runlist continuity | 🟢 GREEN |
| **Volume Cluster Bitmap**| `ntfs_alloc_clusters` searches Record 6 `$Bitmap`, sets bits, writes back | Bit $K$ corresponds to cluster LCN $K$ | Allocation bitmap cached and logged | Corrupt cluster bitmap triggers chkdsk | 🟡 YELLOW |
| **$INDEX_ROOT (0x90)** | Parses header and resident entries; write appends without checking capacity | Resident root node of directory B+tree ($FILE_NAME index) | Manages entries within resident space; promotes on split | Traversed during `NtfsMountVolume` | 🟡 YELLOW |
| **$INDEX_ALLOCATION (0xA0)**| Read support traverses 4096B INDX blocks; **Write insertion missing** | Non-resident 4096B B+tree sub-nodes | Full allocation and insertion into sub-node tree | Traversed for directories $> 20$ files | 🔴 RED |
| **Directory $BITMAP (0xB0)**| Attribute type defined; unmanaged on directory write | Tracks allocated 4096B blocks in `$INDEX_ALLOCATION` | Allocates new block via index bitmap when tree expands | Validated on directory operations | 🔴 RED |
| **Directory B-Tree Topology**| **Blindly appends to root node; corrupts router nodes in large dirs** | B+Tree: Root contains router keys with child VCNs; leaves contain data | Full B+Tree traversal (`indx_find`), node splitting (`indx_insert_into_buffer`)| Collation or flag inversion triggers BSOD 0x24 | 🔴 RED |
| **Filename Collation** | Case-insensitive ASCII substring search; **no collation sorting on insert** | Strict Unicode collation using volume `$UpCase` table (Record 10) | Uses `$UpCase` table for case-insensitive binary collation | Out-of-order entry causes `NtfsFindIndexEntry` panic | 🔴 RED |
| **B-Tree Node Splits** | **Not implemented; aborts write if root slack space is exceeded** | When a node overflows, splits entries at median, promotes divider | `hdr_find_split` splits node, allocates block, updates parent | Required when root node fills up | 🔴 RED |
| **Index Entry Flags** | Hardcoded `flags = 0` (leaf); **omits child VCN in router nodes** | `0x01` = Has Child VCN; `0x02` = End Marker; `0x03` = End + Subnode | Statically sets flags based on whether node is leaf or router | Router node with leaf flag triggers BSOD 0x24 | 🔴 RED |
| **Directory End Marker** | Finds end marker by flag `0x02`; does not maintain child VCN on split | Final entry in node, terminates list, routes $>$ keys to right child | Preserves end marker invariants across all splits and merges | End marker must always be last entry | 🟡 YELLOW |
| **Timestamps (FILETIME)** | 64-bit 100ns FILETIME encoding and decoding since 1601-01-01 | Standard Windows FILETIME format | Updated on metadata write | Validated by Windows kernel | 🟢 GREEN |
| **$LogFile (Journaling)** | In-memory transaction stubs; **no physical logging to Record 2** | Restart area + physical logging of metadata transitions | Full undo/redo logging or marks volume dirty if unsupported | Windows expects clean log or replays journal | 🔴 RED |
| **Volume Dirty Flag** | Does not inspect or update `VOLUME_DIRTY` in Record 3 (`$Volume`) | Bit `0x0001` in `$Volume` attribute `0x70` marks volume dirty | Sets dirty on mount if journal uncommitted; clears on clean unmount | Windows runs `autochk` on boot if dirty flag is set | 🟡 YELLOW |
| **Write Ordering & Safety** | Uncoordinated writes; failed step leaves disk in inconsistent state | Order: MFT Record $\rightarrow$ $MFT::$BITMAP $\rightarrow$ Parent Directory $\rightarrow$ Flush | Strictly ordered writeback with barriers | Torn metadata states trigger protective BugChecks | 🔴 RED |

---

## 2. Detailed Technical Breakdown of Critical Red Items

### Priority 1: MFT Record Allocation & $MFT::$BITMAP Synchronization
- **The Defect:** In `ntfs_mft_alloc_record()`, ATOMS verifies that candidate record $R$ is free in Record 0's `$BITMAP`. However, after selecting record $R$, it writes the record header with `flags = NTFS_FILE_IN_USE (0x0001)` to disk **without ever setting bit $R$ to 1 in Record 0's `$BITMAP` and without flushing the bitmap cluster back to disk**.
- **The Consequence:** When Windows 11 mounts the filesystem, it encounters an MFT record marked `IN_USE` whose allocation bitmap bit is `0` (`FREE`). This is a critical internal contradiction that triggers:
  $$\text{ntfs.sys!NtfsCheckBitmap() } \longrightarrow \text{ KeBugCheckEx(0x24, ...)}$$
  Furthermore, Windows will treat the record as free and overwrite it during early boot (as observed on physical hardware with `EM44C4~1.XML`).
- **Required Architecture:**
  1. Parse Record 0's `$BITMAP` attribute.
  2. Locate target byte $R / 8$ and bit $R \pmod 8$.
  3. Set bit: `byte_val |= (1 << (R % 8))`.
  4. Write the modified bitmap sector to disk via the canonical extent mapper.
  5. Issue hardware flush before returning success.

---

### Priority 2: Directory B-Tree Collation & $UpCase Table
- **The Defect:** In `ntfs_btree_insert()`, ATOMS iterates through the directory entries until it encounters the End Marker (`flags & 0x02`), and then unconditionally shifts bytes to insert the new entry at the very end of the node.
- **The Consequence:**
  - In Record 5, the last entry before the End Marker was `"Windows"`.
  - ATOMS appended `"ATOMS_WRITE_TEST.txt"`.
  - Under Unicode collation, `"ATOMS_WRITE_TEST.txt"` ($0x0041$) must precede `"Windows"` ($0x0057$).
  - When `ntfs.sys` executes a binary search in `NtfsFindIndexEntry`, it asserts $Key_i \le Key_{i+1}$. Detecting $Key_N > Key_{N+1}$ signals index corruption and halts the operating system immediately.
- **Required Architecture:**
  1. Load and cache the volume `$UpCase` table from MFT Record 10 (`$UpCase`).
  2. Implement canonical NTFS Unicode filename collation:
     $$\text{ntfs\_collate\_names(name1, name2, upcase\_table)}$$
  3. Perform binary search / ordered traversal of index entries to locate the exact insertion slot where $Key_{prev} \le Key_{new} < Key_{next}$.
  4. Insert at that exact offset.

---

### Priority 3: Two-Tier Directory Hierarchy & Router Node Downlinks
- **The Defect:** Large directories (such as the Root Directory on any standard Windows 11 installation) are **two-tier B-Trees** containing both `$INDEX_ROOT` (resident router keys) and `$INDEX_ALLOCATION` (non-resident 4096-byte `INDX` blocks). ATOMS assumed all directories are flat single-tier lists and inserted an entry with `flags = 0x0000` (leaf format without a child VCN pointer) directly into `$INDEX_ROOT`.
- **The Consequence:**
  - An entry inside a router node MUST have `flags & 0x01` (`NTFS_INDEX_ENTRY_HAS_SUBNODE`) and carry an 8-byte child VCN at offset `entry->length - 8`.
  - Omitting the child VCN pointer causes `ntfs.sys` to miscalculate the entry trailer and fail index validation.
  - Furthermore, if an entry belongs in a subtree, it must be inserted into the appropriate leaf node inside `$INDEX_ALLOCATION`, not into `$INDEX_ROOT`.
- **Required Architecture:**
  1. Check `idx_hdr->flags & 0x01` (`HAS_LARGE_INDEX`).
  2. If the directory has `$INDEX_ALLOCATION`, traverse down the B-tree by following child VCN pointers until reaching the appropriate leaf `INDX` block.
  3. Insert the entry into the leaf `INDX` block.
  4. If the leaf block exceeds 4096 bytes, execute a B-tree node split (`hdr_find_split`), allocate a new index block via directory `$BITMAP`, move the upper half of entries to the new block, and promote the median key to the parent node with child VCN pointers.

---

### Priority 4: Reallocation Sequence Number Increment
- **The Defect:** When allocating an MFT record slot, ATOMS statically sets `hdr->sequence_number = 1`.
- **The Consequence:** If the slot was previously occupied and deleted by Windows (where `sequence_number` might have been 15), resetting it to 1 causes existing directory references or transaction logs to experience sequence mismatch errors.
- **Required Architecture:**
  - Read the existing on-disk record header at the candidate slot.
  - If it has a valid sequence number $S$, set the new sequence number to $S + 1$.
  - If virgin ($0$), initialize to $1$.

---

### Priority 5: Atomic Transaction Ordering & Flush Semantics
- **The Defect:** ATOMS currently updates metadata structures in an ad-hoc sequence without explicit write-ahead ordering or crash-consistency barriers.
- **Required Architecture:**
  - **Step 1 (Allocation):** Select free MFT record $R$ and free clusters.
  - **Step 2 (Payload & MFT):** Format and write Record $R$ to physical LBA.
  - **Step 3 (MFT Bitmap):** Set bit $R = 1$ in Record 0's `$BITMAP` and write to disk.
  - **Step 4 (Directory Index):** Insert entry into parent directory in sorted collation order and write to disk.
  - **Step 5 (Hardware Barrier):** Issue native `NVMe FLUSH` or ATA `FLUSH CACHE EXT` to ensure all metadata is committed to non-volatile storage.

---

## 3. Road Map to Production Certification

```mermaid
gantt
    title ATOMS OS Native NTFS Engine Implementation Roadmap
    dateFormat  YYYY-MM-DD
    section Phase 1: Core Prerequisites
    $UpCase Table Loader & Unicode Collation :done, p1_1, 2026-09-04, 1d
    $MFT::$BITMAP Read/Modify/Write Engine   :active, p1_2, 2026-09-04, 1d
    Sequence Number Increment Lifecycle      :active, p1_3, 2026-09-04, 1d
    section Phase 2: Index Engine
    Single-Tier Collation-Sorted Insertion   :p2_1, after p1_3, 1d
    Two-Tier B-Tree Traversal Engine         :p2_2, after p2_1, 1d
    INDX Block Split & Promotion Logic       :p2_3, after p2_2, 2d
    section Phase 3: Hardware Validation
    Single-File Resident Write & Read-Back   :p3_1, after p2_3, 1d
    Cross-Boot Validation into Windows 11    :p3_2, after p3_1, 1d
    Multi-File & Directory Stress Testing    :p3_3, after p3_2, 2d
```

This matrix establishes the authoritative blueprint for engineering the complete native ATOMS NTFS Read/Write engine.
