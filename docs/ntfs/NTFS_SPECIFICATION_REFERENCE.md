# NTFS Complete Technical Specification & Architectural Reference
**Document Version:** 1.0.0 — Forensic & Recovery Grade  
**Author:** ATOMS OS Storage & Core Architecture Group  
**Target Architecture:** Microsoft NTFS v3.1 (Windows XP through Windows 11 / Windows Server 2025)  
**Safety Classification:** NON-DESTRUCTIVE / READ-ONLY REFERENCE SPECIFICATION  

---

## Table of Contents
- [01 Volume Layout](#01-volume-layout)
- [02 Boot Sector](#02-boot-sector)
- [03 Clusters & Cluster Geometry](#03-clusters--cluster-geometry)
- [04 Master File Table ($MFT)](#04-master-file-table-mft)
- [05 MFT Records](#05-mft-records)
- [06 FILE Record Header](#06-file-record-header)
- [07 Update Sequence Array (USA / Fixups)](#07-update-sequence-array-usa--fixups)
- [08 Sequence Numbers](#08-sequence-numbers)
- [09 File References (MFT Reference)](#09-file-references-mft-reference)
- [10 Attributes Overview](#10-attributes-overview)
- [11 $STANDARD_INFORMATION (0x10)](#11-standard_information-0x10)
- [12 $FILE_NAME (0x30)](#12-file_name-0x30)
- [13 $DATA (0x80)](#13-data-0x80)
- [14 Resident Data Attributes](#14-resident-data-attributes)
- [15 Non-Resident Data Attributes](#15-non-resident-data-attributes)
- [16 Data Runs & Runlist Encoding](#16-data-runs--runlist-encoding)
- [17 MFT Extent Mapping ($MFT Fragmentation)](#17-mft-extent-mapping-mft-fragmentation)
- [18 $MFT::$BITMAP (Record 0 Bitmap)](#18-mftbitmap-record-0-bitmap)
- [19 Volume Bitmap ($Bitmap, Record 6)](#19-volume-bitmap-bitmap-record-6)
- [20 Directories in NTFS](#20-directories-in-ntfs)
- [21 $INDEX_ROOT (0x90)](#21-index_root-0x90)
- [22 $INDEX_ALLOCATION (0xA0)](#22-index_allocation-0xa0)
- [23 Directory $BITMAP (0xB0)](#23-directory-bitmap-0xb0)
- [24 B-Tree Topology & Multi-Tier Directories](#24-b-tree-topology--multi-tier-directories)
- [25 NTFS Filename Collation Rules ($UpCase)](#25-ntfs-filename-collation-rules-upcase)
- [26 File Creation Lifecycle & Invariants](#26-file-creation-lifecycle--invariants)
- [27 File Deletion Lifecycle & Invariants](#27-file-deletion-lifecycle--invariants)
- [28 Hard Links & Multiple $FILE_NAME Attributes](#28-hard-links--multiple-file_name-attributes)
- [29 $ATTRIBUTE_LIST (0x20)](#29-attribute_list-0x20)
- [30 Journaling ($LogFile & $UsnJrnl)](#30-journaling-logfile--usnjrnl)
- [31 Global Consistency Invariants](#31-global-consistency-invariants)
- [32 Windows ntfs.sys Compatibility & BugCheck 0x24 Triggers](#32-windows-ntfssys-compatibility--bugcheck-0x24-triggers)

---

## 01 Volume Layout
- **Purpose:** Organizes an entire disk partition into linear logical clusters (LCNs).
- **Layout:**
  - `LCN 0`: Volume Boot Record (VBR) / Boot Sector (Sector 0 to Sector 7 on 4KB cluster).
  - `LCN 1 .. N`: Data area containing MFT, system metadata files, and user file data.
  - Middle / Configured offset (e.g. `LCN 786432`): `$MFT` start cluster.
  - End of volume: Backup Boot Sector (exact copy of sector 0).
- **Semantics:** All addresses in NTFS beyond the boot sector are addressed in Logical Cluster Numbers (LCN) relative to the start of the partition.
- **ATOMS Implementation Mapping:** `ntfs_mount()` reads sector 0, parses BPB, calculates `vol->mft_lcn`, and sets base device references.

---

## 02 Boot Sector
- **Purpose:** Identifies filesystem, geometry, sector/cluster sizing, and initial MFT cluster location.
- **On-Disk Layout (512 bytes):**
  - `0x00` (3 bytes): Jump instruction (`EB 52 90`).
  - `0x03` (8 bytes): OEM ID (`"NTFS    "`).
  - `0x0B` (2 bytes): Bytes per sector (512, 1024, 2048, or 4096).
  - `0x0D` (1 byte): Sectors per cluster (typically 8 for 4KB cluster).
  - `0x0E` (7 bytes): Reserved / unused.
  - `0x15` (1 byte): Media descriptor (`0xF8` for fixed disk).
  - `0x28` (8 bytes): Total sectors in partition.
  - `0x30` (8 bytes): Starting LCN of `$MFT`.
  - `0x38` (8 bytes): Starting LCN of `$MFTMirr`.
  - `0x40` (1 byte): Clusters per MFT record (if signed negative `0xF6` = $2^{|-10|} = 1024$ bytes).
  - `0x44` (1 byte): Clusters per Index Buffer (if signed negative `0xF6` = 4096 bytes).
  - `0x48` (8 bytes): Volume Serial Number.
  - `0x1FE` (2 bytes): Boot signature (`0x55, 0xAA`).
- **Invariants:** `magic == "NTFS    "`, `signature == 0xAA55`.
- **ATOMS Mapping:** `NTFS_BootSector` in `ntfs.h:35-52`.

---

## 03 Clusters & Cluster Geometry
- **Purpose:** Fundamental allocation unit for data and non-resident metadata.
- **Offsets/Sizes:** Typically 4096 bytes (8 sectors $\times$ 512 bytes).
- **Semantics:** Cluster index $0$ starts at partition sector $0$. Physical byte offset $= \text{LCN} \times \text{bytes\_per\_cluster}$.
- **Failure Consequences:** Miscalculating sectors-per-cluster shifts all subsequent LBA calculations into foreign data.

---

## 04 Master File Table ($MFT)
- **Purpose:** Central database containing a record for every file and directory on the volume.
- **Structure:** Array of 1024-byte file records.
- **Reserved Records (0 to 15):**
  - `Record 0`: `$MFT` (MFT itself)
  - `Record 1`: `$MFTMirr` (Backup of records 0-3)
  - `Record 2`: `$LogFile` (Transactional logging)
  - `Record 3`: `$Volume` (Volume serial, label, dirty flags)
  - `Record 4`: `$AttrDef` (Attribute definition table)
  - `Record 5`: `.` (Root directory index)
  - `Record 6`: `$Bitmap` (Volume cluster allocation map)
  - `Record 7`: `$Boot` (Volume boot sector duplicate)
  - `Record 8`: `$BadClust` (Bad sector tracking)
  - `Record 9`: `$Secure` (Access Control Lists / Security Descriptors)
  - `Record 10`: `$UpCase` (Unicode uppercase table for collation)
  - `Record 11`: `$Extend` (Optional extensions directory)
  - `Records 12-15`: Reserved for future use.
  - `Records 16+`: Regular user and system files/directories.

---

## 05 MFT Records
- **Purpose:** 1024-byte contiguous block describing a file or directory.
- **Layout:** Header (`0x00..0x37`) followed by Update Sequence Array (`USA`), followed by an ordered list of variable-length attributes, terminated by `0xFFFFFFFF`.

---

## 06 FILE Record Header
- **Layout (48 bytes + USA):**
  - `0x00` (4 bytes): Magic `"FILE"` (`0x454C4946`).
  - `0x04` (2 bytes): `usa_offset` (typically 48 / `0x30`).
  - `0x06` (2 bytes): `usa_count` (sectors per record + 1, e.g. $1024 / 512 + 1 = 3$).
  - `0x08` (8 bytes): `$LogFile` Sequence Number (LSN).
  - `0x10` (2 bytes): Sequence Number (incremented upon reallocation).
  - `0x12` (2 bytes): Hard Link Count (number of directory references).
  - `0x14` (2 bytes): Offset to first attribute (typically 56 / `0x38`).
  - `0x16` (2 bytes): Flags (`0x0001` = In-Use, `0x0002` = Directory).
  - `0x18` (4 bytes): Bytes in use (actual length of record header + attributes + end marker).
  - `0x1C` (4 bytes): Bytes allocated (1024 bytes).
  - `0x20` (8 bytes): Base file record reference (0 if base record).
  - `0x28` (2 bytes): Next attribute instance ID.
  - `0x2A` (2 bytes): Record alignment padding.
  - `0x2C` (4 bytes): MFT Record Number (NTFS 3.1+).

---

## 07 Update Sequence Array (USA / Fixups)
- **Purpose:** Multi-Sector Transfer Protection detecting torn writes / partial disk sector updates.
- **Mechanism:**
  - Before writing to disk, the last 2 bytes of each 512-byte sector (offsets 510 and 1022 in a 1024-byte record) are copied into `usa[1]` and `usa[2]`.
  - A sequence number `usa[0]` is written into both sector trailers.
  - Upon read: If the sector trailer does not match `usa[0]`, the record is torn/corrupt.
  - If it matches, the original 2 bytes are restored from `usa[s+1]` into the sector trailers.
- **Invariants:** `usa_offset + (usa_count * 2) <= first_attribute_offset`.

---

## 08 Sequence Numbers
- **Purpose:** Prevents stale 64-bit file references from accessing reallocated MFT records.
- **Semantics:** 16-bit integer in record header. Incremented each time an MFT record is freed and reallocated. Never 0 in active records.

---

## 09 File References (MFT Reference)
- **Structure (64 bits):**
  - Bits `0..47`: 48-bit MFT Record Number.
  - Bits `48..63`: 16-bit Sequence Number.
- **Invariants:** The sequence number in the reference must match `sequence_number` in the target MFT record. Mismatch yields `STATUS_INVALID_PARAMETER` or `0x24` in Windows.

---

## 10 Attributes Overview
- **Purpose:** Everything in NTFS is an attribute (data, filenames, security, indexes).
- **Structure:**
  - `0x00` (4 bytes): Attribute Type code.
  - `0x04` (4 bytes): Total attribute record length (must be 8-byte aligned).
  - `0x08` (1 byte): Non-resident flag (`0` = resident, `1` = non-resident).
  - `0x09` (1 byte): Name length (in UTF-16 characters; 0 if unnamed).
  - `0x0A` (2 bytes): Offset to name (relative to attribute start).
  - `0x0C` (2 bytes): Flags (`0x0001` = compressed, `0x4000` = encrypted, `0x8000` = sparse).
  - `0x0E` (2 bytes): Attribute instance ID.
- **Ordering Rule:** Attributes within a file record **MUST** be sorted in strictly ascending numerical order by attribute type code (`0x10 < 0x20 < 0x30 < ... < 0xFFFFFFFF`). Unsorted attributes cause instant BSOD `0x24`.

---

## 11 $STANDARD_INFORMATION (0x10)
- **Purpose:** Core POSIX/Win32 file timestamps, file permission flags, security ID, and quota.
- **Resident Value Layout (48 or 72 bytes):**
  - `0x00`: File creation time (64-bit Windows FILETIME: 100ns intervals since 1601).
  - `0x08`: File modification time.
  - `0x10`: MFT record change time.
  - `0x18`: File access time.
  - `0x20`: DOS File permissions (`0x20` = Archive, `0x10` = Directory, etc.).
  - `0x24`: Max version / versions.
  - `0x28`: Class ID.
  - `0x2C`: Owner ID (NTFS 3.0+).
  - `0x30`: Security ID (references `$Secure`).

---

## 12 $FILE_NAME (0x30)
- **Purpose:** Stores the filename, parent directory reference, and file size in directory indexes.
- **Resident Value Layout:**
  - `0x00` (8 bytes): Parent directory file reference (lower 48 bits = record, upper 16 = sequence).
  - `0x08` (8 bytes): Creation time.
  - `0x10` (8 bytes): Modification time.
  - `0x18` (8 bytes): MFT change time.
  - `0x20` (8 bytes): Access time.
  - `0x28` (8 bytes): Allocated size on disk.
  - `0x30` (8 bytes): Real / logical size.
  - `0x38` (4 bytes): Flags (`0x20` = Archive, `0x10` = Directory).
  - `0x3C` (4 bytes): Reparse tag.
  - `0x40` (1 byte): Filename length in UTF-16 characters ($L$).
  - `0x41` (1 byte): Namespace (`0` = POSIX, `1` = Win32, `2` = DOS 8.3, `3` = Win32+DOS).
  - `0x42` ($L \times 2$ bytes): UTF-16LE filename string.

---

## 13 $DATA (0x80)
- **Purpose:** Primary content stream of a file. Default stream has no name (`name_length = 0`).

---

## 14 Resident Data Attributes
- **Semantics:** Attribute content is stored directly within the MFT record when payload $\le$ available slack space (typically $\le 700$ bytes).
- **Resident Header:**
  - `0x10` (4 bytes): `value_length` (exact byte size of payload).
  - `0x14` (2 bytes): `value_offset` (offset from attribute header start to payload).
  - `0x16` (1 byte): `indexed_flag`.
  - `0x17` (1 byte): Padding.
- **Invariants:** `value_offset + value_length <= attribute_length`.

---

## 15 Non-Resident Data Attributes
- **Semantics:** Payload is stored in clusters outside the MFT.
- **Non-Resident Header:**
  - `0x10` (8 bytes): Starting Virtual Cluster Number (VCN) (typically 0).
  - `0x18` (8 bytes): Last VCN (inclusive) ($= \text{cluster\_count} - 1$).
  - `0x20` (2 bytes): `mapping_pairs_offset` (offset to data runs).
  - `0x22` (2 bytes): Compression unit size.
  - `0x24` (4 bytes): Reserved.
  - `0x28` (8 bytes): Allocated size on disk ($= \text{clusters} \times \text{cluster\_size}$).
  - `0x30` (8 bytes): Real / data size.
  - `0x38` (8 bytes): Initialized data size.

---

## 16 Data Runs & Runlist Encoding
- **Purpose:** Variable-length compressed encoding of non-contiguous cluster allocations (extents).
- **Encoding Byte:** High nibble = length of LCN offset field; Low nibble = length of cluster count field.
- **Example:** `0x32 0x01 0x10 0x00 0x00 0x0C` $\rightarrow$ 2 bytes count (`0x1001` = 4097 clusters), 3 bytes LCN offset (`0x0C0000` = LCN 786432).

---

## 17 MFT Extent Mapping ($MFT Fragmentation)
- **Purpose:** Maps Virtual Cluster Numbers (VCN) of `$MFT` itself to physical Logical Cluster Numbers (LCN).
- **Critical Requirement:** `$MFT` is itself a non-resident file described by Record 0's `$DATA` attribute. On real partitions, `$MFT` has multiple extents.
- **Failure Consequence:** Reading or writing an MFT record without extent translation computes incorrect LBAs, destroying data or failing validation.

---

## 18 $MFT::$BITMAP (Record 0 Bitmap)
- **Purpose:** Tracks allocation status of every 1024-byte record in `$MFT`.
- **Location:** Unnamed `$BITMAP` attribute (type `0xB0`) inside **Record 0**.
- **Bit Mapping:** Bit $N$ in byte $N / 8$ represents MFT Record $N$.
  - Bit $= 0$: Record is FREE.
  - Bit $= 1$: Record is ALLOCATED / IN_USE.
- **Update Rule:** Whenever a new record is marked `IN_USE`, its bit in Record 0's `$BITMAP` **MUST BE SET TO 1**. Mismatch triggers Windows BugCheck `0x24`.

---

## 19 Volume Bitmap ($Bitmap, Record 6)
- **Purpose:** Tracks allocation of every cluster across the partition.
- **Bit Mapping:** Bit $K$ in byte $K / 8$ corresponds to LCN $K$.

---

## 20 Directories in NTFS
- **Purpose:** Hierarchical indexing structure mapping filenames to MFT references.
- **Architecture:** Implemented as a B+Tree. Small directories reside entirely in `$INDEX_ROOT`. Large directories spill into `$INDEX_ALLOCATION` blocks governed by a directory `$BITMAP`.

---

## 21 $INDEX_ROOT (0x90)
- **Purpose:** Root node of directory B+tree, resident inside the directory's MFT record.
- **Structure:**
  - Attribute Header (`0x00..0x17`)
  - `NTFS_IndexRootHeader` (`0x18..0x27`):
    - `0x00` (4 bytes): Indexed attribute type (`0x30` = `$FILE_NAME`).
    - `0x04` (4 bytes): Collation rule (`0x01` = `COLLATION_FILE_NAME`).
    - `0x08` (4 bytes): Index block size in bytes (typically 4096).
    - `0x0C` (1 byte): Clusters per index block.
  - `NTFS_IndexHeader` (`0x28..0x37`):
    - `0x00` (4 bytes): `entries_offset` (relative to index header; typically 16 / `0x10`).
    - `0x04` (4 bytes): `total_size` (index header size + entries byte length).
    - `0x08` (4 bytes): `allocated_size`.
    - `0x0C` (1 byte): Flags (`0x01` = `HAS_LARGE_INDEX`, meaning `$INDEX_ALLOCATION` exists).

---

## 22 $INDEX_ALLOCATION (0xA0)
- **Purpose:** Non-resident attribute holding 4096-byte B-tree sub-nodes (`INDX` blocks).
- **Structure:** Sequence of 4096-byte blocks, each with `"INDX"` magic, USA fixup array, and child `NTFS_IndexEntry` records.

---

## 23 Directory $BITMAP (0xB0)
- **Purpose:** Tracks allocation of 4096-byte blocks in `$INDEX_ALLOCATION`.
- **Mapping:** Bit $B$ represents the $B$-th 4096-byte index block (VCN $B \times \text{clusters\_per\_block}$).

---

## 24 B-Tree Topology & Multi-Tier Directories
- **Invariants:**
  1. Small Directory: Only `$INDEX_ROOT` exists. All entries are leaf entries (`flags = 0x00`).
  2. Large Directory: Both `$INDEX_ROOT` and `$INDEX_ALLOCATION` exist (`idx_hdr->flags & 0x01 == 1`).
  3. Intermediate Nodes: Entries in `$INDEX_ROOT` act as **routing dividers**. Every entry in `$INDEX_ROOT` (including the End Marker) must have `flags & 0x01` and an 8-byte sub-node VCN pointing to a child INDX block.
  4. End Marker: The final entry in every node has `flags & 0x02` (`NTFS_INDEX_ENTRY_LAST`), zero key length, and routes all keys greater than the preceding entry to the rightmost child VCN.
- **Corruption Trigger:** Appending an entry with `flags = 0` (no child VCN) into an `$INDEX_ROOT` that has `$INDEX_ALLOCATION` violates the B-Tree routing topology, corrupting the tree.

---

## 25 NTFS Filename Collation Rules ($UpCase)
- **Collation Algorithm:**
  1. Names are compared character-by-character.
  2. Each UTF-16 character is mapped to uppercase via the volume's `$UpCase` table (Record 10).
  3. Comparison is based on the numeric value of the uppercase UTF-16 code units.
  4. If uppercase characters match, case-sensitive comparison resolves ties.
- **Invariants:**
  $$\text{Entry}[i] < \text{Entry}[i+1]$$
- **Failure Consequence:** Out-of-order index entries cause binary searches to terminate without finding existing files and cause `ntfs.sys` to throw BugCheck `0x24`.

---

## 26 File Creation Lifecycle & Invariants
An atomic creation of a file requires updating **5 distinct metadata regions**:
1. **$MFT Allocation:** Find free record $R \ge 16$ via Record 0 `$BITMAP`. Set bit $R = 1$ in `$BITMAP`.
2. **Record Initialization:** Zero 1024-byte buffer. Set `'FILE'` magic, sequence number, flags `0x0001`, USA offsets.
3. **Attribute Construction:** Append `$STANDARD_INFORMATION` (0x10), `$FILE_NAME` (0x30), `$DATA` (0x80), and `$END` (0xFFFFFFFF) in strictly ascending order.
4. **Directory B-Tree Insertion:** Insert `$FILE_NAME` key into parent directory's B-tree in strictly sorted collation position. If node splits are required, balance B-tree and update parent router keys.
5. **USA Application & Flush:** Apply sequence fixups to record buffers and flush to physical storage.

---

## 27 File Deletion Lifecycle & Invariants
1. Remove entry from parent directory index. Rebalance B-tree if node underflows.
2. If hard link count reaches 0, free allocated clusters in `$Bitmap` (Record 6).
3. Clear `NTFS_FILE_IN_USE` flag in MFT record header. Increment `sequence_number`.
4. Clear bit $R$ in Record 0's `$BITMAP`.

---

## 28 Hard Links & Multiple $FILE_NAME Attributes
- A file with multiple names has multiple `$FILE_NAME` attributes in its MFT record.
- Typically contains one Win32 name and one DOS 8.3 short name.
- `hard_link_count` in MFT record header equals the total number of `$FILE_NAME` attributes.

---

## 29 $ATTRIBUTE_LIST (0x20)
- **Purpose:** Used when a file's attributes cannot fit within a single 1024-byte MFT record.
- **Mechanism:** The base record contains `$ATTRIBUTE_LIST`, which maps attribute types to external extension MFT records.

---

## 30 Journaling ($LogFile & $UsnJrnl)
- **Purpose:** Redo/undo transactional logging guaranteeing filesystem consistency across system crashes.
- **Invariants:** If an unlogged metadata write occurs while the log transaction sequence number (LSN) is stale, Windows flags the volume dirty.

---

## 31 Global Consistency Invariants
1. **Allocation Equivalence:** Record $N$ marked `IN_USE` $\iff$ `$MFT::$BITMAP` bit $N == 1$.
2. **Cluster Map Equivalence:** Cluster $C$ allocated in non-resident runlist $\iff$ `$Bitmap` bit $C == 1$.
3. **Parent-Child Link Equivalence:** Index entry in dir $P$ points to record $C \iff$ Record $C$ contains `$FILE_NAME` attribute pointing to parent $P$.
4. **Strict Collation:** All index entries within any index node must be sorted monotonically by `$UpCase` code points.

---

## 32 Windows ntfs.sys Compatibility & BugCheck 0x24 Triggers
The Windows kernel filesystem driver (`ntfs.sys`) triggers `KeBugCheckEx(0x00000024)` whenever:
1. An index node fails collation sort verification during binary search (`NtfsFindIndexEntry`).
2. An MFT record marked `IN_USE` has bit `0` in `$MFT::$BITMAP` during allocation validation (`NtfsCheckBitmap`).
3. An index entry in `$INDEX_ROOT` has missing child VCN pointers while `HAS_LARGE_INDEX` is set (`NtfsCheckIndex`).
4. Attribute headers are not in strictly ascending numerical order (`NtfsFindAttribute`).
5. Sector trailer words do not match the Update Sequence Number (`NtfsApplyFixup`).
