# ATOMS OS — Native NTFS Engine Architecture Specification
**Document ID:** `NTFS-ARCH-V1.0`  
**Classification:** Core Operating System Storage Architecture  
**Hardware Target:** ASUS PRIME B750M-K (Intel Core i3-14100F, Haswell/RaptorLake LGA1700)  
**Storage Subsystem:** Native NVMe 1.4 + SATA AHCI Block Layer  
**Target Filesystem:** Microsoft NTFS 3.1 (Windows XP through Windows 11)  
**Authors:** Developer A (ATOMS Architecture) & Developer B (Forensic Storage)  
**Date:** 2026-09-04  

---

## 1. Architectural Mission & Principles

The ATOMS OS Native NTFS Engine is designed to achieve **100% bidirectional interoperability** with Microsoft Windows 11 on bare-metal hardware without relying on third-party user-mode utilities or unstable heuristics.

### Core Architectural Invariants
1. **Zero Speculative Writes:** No disk sector may be modified unless every prerequisite structural dependency is verified and committed in strict transactional order.
2. **Strict B-Tree Collation Invariance:** Every directory entry insertion must be placed in exact lexicographical position according to the volume's `$UpCase` table.
3. **MFT Allocation Synchronization:** No MFT record may be marked `IN_USE` on disk without its corresponding bit in `$MFT::$BITMAP` being atomically set to `1` and flushed.
4. **Multi-Extent MFT Awareness:** All MFT record lookups and updates must be translated through the non-resident extent map of `$MFT::$DATA`. Contiguous MFT layout must never be assumed.
5. **Hardware Cache Barrier Enforcement:** Every mutation transaction must conclude with a hardware cache flush (`NVMe FLUSH` or ATA `FLUSH CACHE EXT`).

---

## 2. Layered Subsystem Architecture

```
+-------------------------------------------------------------------------------+
|                             ATOMS USERSPACE / VFS                             |
|          sys_open(), sys_read(), sys_write(), sys_mkdir(), sys_unlink()       |
+-------------------------------------------------------------------------------+
                                       │
                                       ▼
+-------------------------------------------------------------------------------+
|                       ATOMS VFS NTFS ADAPTER LAYER                            |
|             ntfs_vfs_lookup(), ntfs_vfs_read(), ntfs_vfs_write()              |
+-------------------------------------------------------------------------------+
                                       │
                                       ▼
+-------------------------------------------------------------------------------+
|                     NTFS TRANSACTION & MUTATION ENGINE                        |
|   Atomic write-ahead coordinator: MFT Alloc -> Payload -> Index -> Flush      |
+-------------------------------------------------------------------------------+
       │                               │                               │
       ▼                               ▼                               ▼
+---------------+              +---------------+              +---------------+
|  MFT MANAGER  |              |  INDEX ENGINE |              | ALLOC ENGINE  |
| - Record alloc|              | - $INDEX_ROOT |              | - Clus alloc  |
| - $MFT::$BMP  |              | - $INDEX_ALLOC|              | - Vol $Bitmap |
| - Sequence num|              | - B-Tree split|              | - Runlists    |
| - Extent map  |              | - $UpCase sort|              | - LCN/VCN     |
+---------------+              +---------------+              +---------------+
       │                               │                               │
       └───────────────────────┬───────┴───────────────────────────────┘
                               ▼
+-------------------------------------------------------------------------------+
|                         NTFS ATTRIBUTE ENGINE                                 |
|     $10 STANDARD_INFO | $20 ATTR_LIST | $30 FILE_NAME | $80 DATA              |
|             Resident / Non-Resident Handler & USA Fixup Generator             |
+-------------------------------------------------------------------------------+
                                       │
                                       ▼
+-------------------------------------------------------------------------------+
|                         SECTOR CACHE & I/O BUFFER                             |
|             Coalesced read/write blocks, dirty buffer management              |
+-------------------------------------------------------------------------------+
                                       │
                                       ▼
+-------------------------------------------------------------------------------+
|                       BLOCK DEVICE HAL (NVMe / AHCI)                          |
|         nvme0n1 (WD Blue SN5000) / ahci0 (SATA SSD) with hardware flush       |
+-------------------------------------------------------------------------------+
```

---

## 3. Subsystem Detailed Responsibilities

### 3.1 MFT Manager (`ntfs_mft.c` / MFT Subsystem)
- **Canonical Address Translation:** Implements `ntfs_mft_record_to_physical_lba(vol, record_num, &lba, &ext_idx, &vcn, &lcn)` using the decoded extent map of Record 0's `$DATA` runlist.
- **MFT Record Allocation:**
  - Traverses Record 0's `$BITMAP` (both resident and non-resident runlist clusters).
  - Identifies the first free bit $R \ge 16$.
  - Reads candidate record $R$ from physical disk.
  - Reads previous `sequence_number` $S$ and calculates $S_{new} = (S > 0) ? (S + 1) : 1$.
  - Writes formatted 1024-byte record to physical LBA.
  - Flips bit $R$ to `1` in Record 0's `$BITMAP` buffer.
  - Computes USA fixup for the bitmap sector and writes to disk.
- **MFT Record Deallocation:**
  - Clears `NTFS_FILE_IN_USE` flag in record header.
  - Increments `sequence_number`.
  - Clears bit $R$ to `0` in Record 0's `$BITMAP`.

### 3.2 Collation & $UpCase Engine (`ntfs_collate.c`)
- **Table Initialization:** Loads the 128 KB Unicode uppercase table from MFT Record 10 (`$UpCase`).
- **Collation Algorithm:**
  ```c
  int ntfs_collate_names(const uint16_t* name1, uint8_t len1,
                         const uint16_t* name2, uint8_t len2,
                         const uint16_t* upcase_table) {
      uint8_t min_len = (len1 < len2) ? len1 : len2;
      for (uint8_t i = 0; i < min_len; i++) {
          uint16_t c1 = upcase_table ? upcase_table[name1[i]] : name1[i];
          uint16_t c2 = upcase_table ? upcase_table[name2[i]] : name2[i];
          if (c1 != c2) return (int)c1 - (int)c2;
      }
      return (int)len1 - (int)len2;
  }
  ```
- Guarantees exact parity with Microsoft `ntfs.sys!NtfsCollateNames`.

### 3.3 Directory B-Tree Index Engine (`ntfs_index.c`)
- **Single-Tier Directories ($INDEX_ROOT Only):**
  - Searches entries array for exact insertion index $k$ where $Key_{k-1} \le Key_{new} < Key_k$.
  - If slack space permits ($bytes\_in\_use + entry\_len \le 1024$), shifts trailing bytes forward, inserts entry, updates sizes, and writes back.
  - If slack space is insufficient: triggers **Index Root Split** $\rightarrow$ converts directory to Two-Tier B-Tree.
- **Two-Tier Directories ($INDEX_ROOT + $INDEX_ALLOCATION):**
  - Traverses `$INDEX_ROOT` router keys using binary search.
  - Follows child VCN pointer down to leaf 4096-byte `"INDX"` block.
  - Inserts entry in sorted collation position inside leaf `"INDX"` block.
  - If leaf block exceeds 4096 bytes: triggers **INDX Node Split** (`hdr_find_split`), allocates new index block via directory `$BITMAP`, moves upper half of entries, and promotes divider key to parent router node.

### 3.4 Cluster Allocation Engine (`ntfs_alloc.c`)
- Reads Volume Bitmap (Record 6 `$DATA`).
- Implements best-fit / first-fit free cluster search for requested cluster count.
- Flips bits to `1`, commits bitmap sectors, and generates compressed data runs.

### 3.5 Transaction & Hardware Flush Coordinator (`ntfs_txn.c`)
- Coordinates multi-phase writes to guarantee crash safety:
  $$\text{Stage 1: Pre-Allocation} \longrightarrow \text{Stage 2: Payload Write} \longrightarrow \text{Stage 3: Bitmap Commit} \longrightarrow \text{Stage 4: Index Link} \longrightarrow \text{Stage 5: Hardware Flush}$$
- If any stage encounters an I/O fault, cleanly rolls back prior stages without leaving orphaned metadata.

---

## 4. Hardware Verification Gate

All modifications must pass the formal pre-flight cycle before bare-metal testing:
1. Clean Clang compilation with zero warnings (`-Wall -Wextra`).
2. Pure UEFI QEMU pre-flight verification.
3. Live bare-metal PXE deployment on ASUS B750M-K.
4. Binary PASS/FAIL forensic telemetry confirmation over UDP 9999.
