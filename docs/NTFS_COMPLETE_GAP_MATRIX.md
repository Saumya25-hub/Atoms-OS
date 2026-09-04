# ATOMS OS — Complete NTFS Gap Matrix & Compatibility Audit
**Document ID:** `NTFS-GAP-MATRIX-V2.0`  
**Target Architecture:** Native ATOMS OS Read/Write NTFS Engine  
**Hardware Target:** ASUS PRIME B750M-K (Intel Core i3-14100F, WD Blue SN5000 NVMe 500GB SSD)  
**Reference Implementations:** Microsoft NTFS 3.1 (`ntfs.sys`), Linux Kernel 6.x (`fs/ntfs3`), NTFS-3G  
**Classification:** STRICT SUBSYSTEM COMPARATIVE AUDIT  
**Date:** 2026-09-04  

---

## 1. Primary Volume System Files Matrix

| System File | MFT Rec | ATOMS Current State | NTFS 3.1 Specification | Linux ntfs3 / Microsoft Behavior | Classification |
| :--- | :---: | :--- | :--- | :--- | :---: |
| **$Boot** | N/A (LBA 0) | Full BPB parse, OEM ID check, sector/cluster calculation, backup boot sector probe | 512B VBR at LBA 0 + mirror at volume end. Jump opcode, BPB parameters, 0xAA55 signature | Validates primary and backup VBR, verifies sector/cluster bounds strictly | 🟢 GREEN |
| **$MFT** | 0 | Record 0 parsed, non-resident $DATA runlist decoded into `NTFS_ExtentMap`. Multi-extent canonical mapping | Record 0 describes entire MFT via runlists; self-referential attribute definitions | Full dynamic runlist lookup (`ntfs_bmap`); supports multiple MFT extents | 🟢 GREEN |
| **$MFTMirr** | 1 | Mirror LCN located and read on primary failure; write synchronization not implemented | Duplicate copy of first 4 MFT records (0-3) at mirror LCN for recovery | Mirrors records 0-3 on every modification to system metadata | 🟡 YELLOW |
| **$LogFile** | 2 | In-memory transaction ID tracking; physical restart areas and redo/undo logging not committed | Circular transaction log (minimum 2MB-64MB). Stores LSN, restart areas, transaction records | Parses restart areas, replays uncommitted transactions on mount, marks dirty if torn | 🔴 RED |
| **$Volume** | 3 | Reads volume serial and name; `VOLUME_DIRTY` flag (0x0001) checking/setting stubbed | Contains $VOLUME_NAME and $VOLUME_INFORMATION (OS version, dirty flag 0x0001) | Sets dirty flag on write mount; clears only on clean unmount | 🟡 YELLOW |
| **$AttrDef** | 4 | Hardcoded attribute definitions ($10..$FF) matching standard NTFS table | Attribute definition table mapping type codes to names, flags, and size constraints | Loaded during mount to validate attribute types and flags | 🟢 GREEN |
| **Root Directory** | 5 | Read/enum/lookup working. Single-tier and two-tier leaf insertion with B-tree traversal | Root of namespace; small dirs resident in $INDEX_ROOT, large dirs use $INDEX_ALLOCATION | B+tree management with INDX blocks, allocation bitmap, and node splitting | 🟢 GREEN |
| **$Bitmap** | 6 | Non-resident $DATA stream decoded; `ntfs_alloc_clusters` searches and marks bits | Cluster allocation bitmap for entire volume; bit $K=1$ denotes allocated cluster $K$ | Allocation bitmap cached, updated on cluster allocation/free, flushed with barrier | 🟡 YELLOW |
| **$BadClus** | 8 | Discovered during bootstrap; read-only access; remapping defective blocks not implemented | Tracks bad clusters in non-resident $DATA stream named `$Bad` | Remaps defective sectors on I/O failure | 🟡 YELLOW |
| **$Secure** | 9 | Discovered; security ID checking bypassed (all files granted full access) | Contains security descriptors, owner SIDs, and ACLs in `$SDS`, `$SDH`, `$SII` | Enforces Windows NT ACLs and security descriptors | 🟡 YELLOW |
| **$UpCase** | 10 | Fully read into memory (65,536 entries, 128KB) from Record 10; used for Unicode collation | 128KB table mapping each UTF-16 character to its uppercase equivalent | Mandatory for case-insensitive binary search in directory B-trees | 🟢 GREEN |
| **$Extend** | 11 | Discovered; optional metadata directory ($ObjId, $Quota, $Reparse, $UsnJrnl) unmounted | Container directory for optional volume extensions | Mounted on demand by Windows kernel services | 🟡 YELLOW |

---

## 2. MFT Subsystem Gap Matrix

| MFT Component | ATOMS State | Invariant Requirement | Failure Mode if Violated | Classification |
| :--- | :--- | :--- | :--- | :---: |
| **File Record Header** | 48B header (`FILE`, USA offset 48, count 3, 8-byte aligned attributes) | Standard 48-byte header; bytes_in_use <= bytes_allocated; 8-byte alignment | Header corruption triggers BSOD 0x24 / unreadable record | 🟢 GREEN |
| **USA / Fixup (MFT)** | Full USA verification on read (`apply_fixup`), full generation on write (`write_raw`) | Multi-sector write protection; sequence number placed in sector trailers | Bad fixup triggers corrupted record drop | 🟢 GREEN |
| **USA / Fixup (INDX)**| Full USA verification on read (`indx_validate`), full generation on write | 4096-byte INDX buffer (8 sectors); USA count = 9 words | Corrupted index block panic in `NtfsFindIndexEntry` | 🟢 GREEN |
| **Sequence Numbers** | Reads existing on-disk sequence; reallocates with $S_{old} + 1$; virgin uses $S=1$ | Stored in upper 16 bits of directory `file_reference`; monotonically increments | Sequence mismatch causes Windows to reject file reference | 🟢 GREEN |
| **Hard-Link Count** | Initialized to 1 on create; decremented on unlink; record freed when count reaches 0 | Number of directory entries referencing this MFT record | Dangling references or leaked storage | 🟢 GREEN |
| **Used / Allocated Size**| Dynamically computed from attribute end; padded to 8-byte alignment; alloc=1024B | `bytes_in_use` must accurately reflect distance to end marker `0xFFFFFFFF` | Size mismatch halts parser | 🟢 GREEN |
| **Attributes Ordering** | Strict sorting: `$10 (StdInfo) < $30 (FileName) < $80 (Data) < $FF (End)` | Attributes within record MUST appear in strictly ascending type code order | Out-of-order attributes trigger instant BSOD 0x24 | 🟢 GREEN |
| **Attribute List ($20)**| Resident attribute list parsing supported; creation of multi-record files unneeded | Required when attributes exceed 1024 bytes (slack space) | Overflow if file has many streams | 🟡 YELLOW |
| **$MFT::$BITMAP Sync** | Reads and modifies bit in Record 0 `$BITMAP`; writes sector to physical disk; flushes | Bit $N=1$ indicates record $N$ allocated; bit $N=0$ indicates free | Bit=0 with Record=IN_USE triggers Windows 0x24 crash | 🟢 GREEN |
| **MFT Extent Mapping** | Canonical `ntfs_mft_record_to_physical_lba()` translates VCN to LCN for ALL reads/writes | Supports multi-extent non-contiguous MFT storage | Fragmented MFT reads wrong sector | 🟢 GREEN |
| **Record Allocation** | Scans above system area ($\ge 1024$); verifies bit=0 in bitmap AND virgin/freed on disk | Safe slot selection; avoids overwriting active system records | Overwriting system file destroys OS | 🟢 GREEN |
| **Record Reuse** | Safely detects freed records (`FILE` with `flags & 1 == 0`); updates sequence number | Replaces deleted slots; preserves filesystem density | Leaked MFT space | 🟢 GREEN |

---

## 3. Directory & B-Tree Subsystem Gap Matrix

| Directory Component | ATOMS State | Invariant Requirement | Failure Mode if Violated | Classification |
| :--- | :--- | :--- | :--- | :---: |
| **$INDEX_ROOT (0x90)** | Parses resident entries; determines single-tier vs two-tier via `flags` | Resident root node of directory; contains router keys in two-tier trees | Corrupt root breaks folder access | 🟢 GREEN |
| **$INDEX_ALLOCATION (0xA0)**| Traverses 4096B INDX blocks via non-resident runlist; reads target blocks | Non-resident container for B+tree sub-nodes | Missing allocation breaks large folders | 🟢 GREEN |
| **Directory $BITMAP (0xB0)**| Queries allocation bit; manages free INDX blocks in `$INDEX_ALLOCATION` | Tracks allocated 4KB INDX blocks within directory | Leaked or overlapping index blocks | 🟢 GREEN |
| **INDX Buffers** | 4096-byte buffers with `"INDX"` magic, USA, LSN, and entries offset | Must conform to `NTFS_IndexBlockHeader` with valid sector fixups | Fixup failure causes folder corruption | 🟢 GREEN |
| **VCN References** | Follows down-link VCNs in router entries to locate child INDX blocks | Multi-level tree navigation down to leaf level | Traversal failure loses file | 🟢 GREEN |
| **Child Pointers** | Preserved on router entries in `$INDEX_ROOT` and non-leaf INDX blocks | Trailing 8 bytes in entries with `flags & 0x01` | Missing child pointer breaks B-tree | 🟢 GREEN |
| **Leaf Entries** | Pure leaf entries created with `flags = 0x0000` (no child VCN trailer) | Entries in leaf INDX blocks must omit child VCN | Leaf with child flag corrupts parser | 🟢 GREEN |
| **End Markers** | Entry with `flags = 0x02` (`NTFS_INDEX_ENTRY_LAST`) terminates entry stream | Every index node must terminate with end marker | Runaway loop past buffer boundary | 🟢 GREEN |
| **Filename Collation** | Full two-pass collation: Unicode upcase comparison via `$UpCase` + length + tie-breaker | Strict binary ordering: $Key_i < Key_{i+1}$ | Out-of-order entry triggers BSOD 0x24 | 🟢 GREEN |
| **B-Tree Insertion** | Sorted insertion: shifts existing entries, inserts new leaf, updates sizes and USA | Preserves ascending collation order within block | Inverted keys trigger BSOD 0x24 | 🟢 GREEN |
| **B-Tree Node Split** | Implemented for leaf blocks: allocates new INDX block, moves half, promotes median | When block exceeds 4096B, splits cleanly and updates parent | Full block rejection without split | 🟢 GREEN |
| **Directory Lookup** | Binary search / ordered traversal of entries in root and sub-nodes | Locates file reference by name in $O(\log N)$ time | File not found error | 🟢 GREEN |
| **Directory Deletion** | Replaces entry with shifted tail, updates `total_size`, preserves end marker | Removes entry, balances tree if underflowing | Leaked directory entries | 🟡 YELLOW |

---

## 4. Data Streams & Allocation Subsystem Gap Matrix

| Data Feature | ATOMS State | Invariant Requirement | Failure Mode if Violated | Classification |
| :--- | :--- | :--- | :--- | :---: |
| **Resident $DATA** | Files $\le 256$ bytes stored directly in MFT record; zero clusters allocated | Payload fits within MFT record slack; `non_resident = 0` | MFT record overflow | 🟢 GREEN |
| **Non-Resident $DATA** | Allocates clusters via `$Bitmap`, encodes runlist via `ntfs_encode_data_runs` | Clusters allocated on disk; mapped via nibble-compressed runlists | Unmapped clusters or data loss | 🟢 GREEN |
| **Runlist Compression** | Variable-length nibble encoding of LCN differences and cluster counts | Correctly formats byte header `(lcn_bytes << 4) \| len_bytes` | Malformed runlist crashes parser | 🟢 GREEN |
| **Cluster Allocation** | Scans Record 6 `$Bitmap`, finds contiguous free clusters, sets bits, writes back | Allocates free space without colliding with existing data | Cluster collision destroys user data | 🟢 GREEN |
| **Sparse Files** | Runlist decoder handles sparse extents (`lcn_start = -1`); creation unneeded | Clusters omitted for zero blocks; synthesized on read | Reading sparse file returns garbage | 🟡 YELLOW |
| **Compression** | LZNT1 decompression supported on read; compressed write unneeded | 16-cluster compression units; compressed in-place | Reading compressed file fails | 🟡 YELLOW |
| **Valid Data Length** | Enforced: `initialized_size` tracks written bytes; reads past VDL return zeros | Data between VDL and EOF must return zero bytes | Leaking uninitialized disk data | 🟢 GREEN |

---

## 5. Consistency, Transactions & Recovery Gap Matrix

| Mechanism | ATOMS State | Invariant Requirement | Failure Mode if Violated | Classification |
| :--- | :--- | :--- | :--- | :---: |
| **Metadata Ordering** | Ordered: Payload/Data $\rightarrow$ $MFT::$BITMAP $\rightarrow$ MFT Record $\rightarrow$ Dir Index $\rightarrow$ Flush | Write-ahead ordering prevents orphan directory entries | Inconsistent state after crash | 🟢 GREEN |
| **Hardware Flush** | Explicit `nvme_flush(1)` issued after metadata commit | Enforces non-volatile barrier before acknowledging success | Write reordering in volatile cache | 🟢 GREEN |
| **Rollback Capability** | On failure, clears MFT bitmap bit, zeroes MFT record, frees clusters, flushes | Failed operation must leave disk 100% untouched/consistent | Partial/torn file leaves disk dirty | 🟢 GREEN |
| **Crash Consistency**| Scoped: Atomic ordering minimizes crash window; full undo/redo requires $LogFile | System recovers cleanly with `chkdsk /f` without BSOD | BugCheck 0x24 on uncommitted state | 🟡 YELLOW |
| **$LogFile Journaling**| Physical logging to Record 2 not yet implemented; marked clean on unmount | Full WAL (write-ahead log) replaying uncommitted operations | Windows triggers autochk replay | 🔴 RED |

---

## 6. Windows Interoperability Verification Matrix

| Interoperability Scenario | Verification Step | Expected Result | Hardware Result | Status |
| :--- | :--- | :--- | :--- | :---: |
| **Read Windows NTFS** | Mount Windows 11 volume on ASUS B750M-K; read system files | Clean mount, correct BPB, readable MFT | 100% PASS (Screen 19:49:19) | 🟢 GREEN |
| **ATOMS File Creation** | Create `/ATOMS_TEST.txt` (resident) with valid metadata | MFT alloc, bitmap sync, sorted index insertion | Tested & verified in engine | 🟢 GREEN |
| **ATOMS Verification** | Read back `/ATOMS_TEST.txt` within ATOMS OS | Identical byte content, valid MFT record | Verified | 🟢 GREEN |
| **Windows Mount** | Reboot physical hardware into Windows 11 | Windows boots cleanly without BugCheck 0x24 | Target certification | 🟡 PENDING HW RUN |
| **Windows Read File** | Open `/ATOMS_TEST.txt` in Windows 11 Notepad / PowerShell | File visible in Explorer, readable, correct size | Target certification | 🟡 PENDING HW RUN |
| **Windows Reboot** | Reboot Windows 11, boot back into ATOMS OS | Filesystem remains healthy, no autochk errors | Target certification | 🟡 PENDING HW RUN |
