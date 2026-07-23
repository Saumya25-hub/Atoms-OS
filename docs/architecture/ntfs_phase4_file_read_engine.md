# ATOMS OS — NTFS Phase 4: File Read Engine Architecture & Certification Document

This document records the design, implementation, read pipeline, extent traversal models, sparse zero synthesis, initialized-size protection, read caching, test evidence, and Phase 5 handoff specification for **NTFS Phase 4 — File Read Engine** in ATOMS OS.

---

## 1. Phase Status

| Sub-Phase | Description | Status |
| :--- | :--- | :--- |
| **4A** | File Lookup & File Handle Foundation | **COMPLETE** |
| **4B** | Resident File Reads | **COMPLETE** |
| **4C** | Non-Resident File Reads | **COMPLETE** |
| **4D** | Offset / Range Reads | **COMPLETE** |
| **4E** | Fragmented & Sparse Reads | **COMPLETE** |
| **4F** | Read Cache Engine | **COMPLETE** |

**Overall Phase 4 Status:** **PASS (100% CERTIFIED)**

---

## 2. Architecture & Pipeline

```
[Validated NTFS_FileRecord (Phase 2)]
            │
            ▼
[ntfs_file_open_by_record()] ──► Select Unnamed $DATA Attribute (Phase 3)
            │
            ▼
┌───────────────────────────┴───────────────────────────┐
│                                                       │
▼                                                       ▼
[Resident Stream (4B)]                    [Non-Resident Stream (4C)]
│                                                       │
│                                                       ▼
│                                         [Offset & Length EOF Clamping]
│                                                       │
│                                                       ▼
│                                         [Initialized Size Check (4E)]
│                                         (Fills uninitialized gap with 0s)
│                                                       │
│                                                       ▼
│                                         [Logical VCN to Extent Lookup]
│                                         (ntfs_extent_map_lookup)
│                                                       │
│                                ┌──────────────────────┴──────────────────────┐
│                                │                                             │
│                                ▼                                             ▼
│                         [Sparse Extent (4E)]                          [Allocated Extent (4C)]
│                         (Zero I/O, Fill 0s)                                  │
│                                                                              ▼
│                                                                       [Physical LCN Calc & Sector LBA]
│                                                                              │
│                                                                              ▼
│                                                                       [64-Entry LRU Read Cache (4F)]
│                                                                       (ntfs_read_sector_cached)
│                                                                              │
│                                                                              ▼
└───────────────────────────────────────┬──────────────────────────────────────┘
                                        │
                                        ▼
                                [Caller Buffer]
```

---

## 3. Phase 1–3 Contracts Reused

- **Phase 1 Geometry (`NTFS_VOLUME`):** `bytes_per_sector`, `bytes_per_cluster`, `sectors_per_cluster`, `total_sectors`, `total_clusters`.
- **Phase 2 MFT Core (`NTFS_FileRecord`):** Uses validated records (`rec->state == NTFS_RECORD_STATE_VALIDATED`) with USA/fixups applied and sector trailers restored.
- **Phase 3 Attribute Engine:** `ntfs_attr_find()`, `ntfs_attr_get_resident_value()`, `ntfs_decode_data_runs()`, `ntfs_extent_map_lookup()`, and primary `$MFT::$DATA` bootstrap map.

---

## 4. File Context & Lifetime (`NTFS_File`)

```c
typedef struct {
    NTFS_VOLUME*     vol;
    uint32_t         record_number;
    NTFS_FileRecord* record;

    bool             is_directory;
    bool             has_data;
    bool             non_resident;
    bool             is_compressed;
    bool             is_encrypted;

    uint64_t         data_size;        // Authoritative logical file size
    uint64_t         allocated_size;
    uint64_t         initialized_size;

    const uint8_t*   resident_data;
    uint32_t         resident_len;

    NTFS_ExtentMap   extent_map;
} NTFS_File;
```
- **Opening:** `ntfs_file_open_by_record(vol, record_number)` loads the record, finds the default unnamed `$DATA` stream, parses resident payload or non-resident mapping pairs, and sets authoritative sizes.
- **Closing:** `ntfs_file_close(file)` frees the underlying `NTFS_FileRecord`, extent map, and context.

---

## 5. Resident Read Contract (4B)

- Content resides inside the MFT record payload (`resident_data`).
- `ntfs_file_read()` clamps `len` to `data_size - offset`.
- Copies bytes directly from `resident_data + offset` into the destination buffer.
- Offset at or beyond EOF returns `0` bytes.

---

## 6. Non-Resident Read Contract (4C, 4D)

1. Receives logical `offset` and requested `len`.
2. Clamps `len` to `data_size - offset`. Returns `0` if `offset >= data_size`.
3. Converts byte offset to logical `vcn = offset / bytes_per_cluster` and `intra_cluster_offset`.
4. Resolves `vcn` using `ntfs_extent_map_lookup()`.
5. Computes physical LCN: `phys_lcn = extent.lcn_start + (vcn - extent.vcn_start)`.
6. Converts physical LCN to sector LBA and reads sectors via `ntfs_read_sector_cached()`, using a 512-byte sector bounce buffer for unaligned byte offsets.

---

## 7. Fragmented Read Model (4E)

- Read requests spanning multiple non-contiguous physical extents are automatically split into extent-bounded chunks.
- Maintains logical VCN continuity while recalculating physical LCN for each extent chunk.

---

## 8. Sparse Read Semantics (4E)

- Extents with `is_sparse = true` (`lcn_start = -1`) trigger automatic logical zero synthesis.
- Fills destination buffer with zeros using `memset` without performing physical disk I/O.

---

## 9. Initialized-Size Protection (Security Requirement)

- For non-resident files where `initialized_size < data_size`, any byte offset in the range `initialized_size <= offset < data_size` is treated as uninitialized storage.
- The engine synthesizes logical zero bytes for uninitialized regions, preventing exposure of stale unallocated disk contents.

---

## 10. Read Cache Architecture (4F)

- **Structure:** 64-entry LRU sector cache (`NTFS_ReadCache`) embedded in `NTFS_VOLUME`.
- **API:** `ntfs_read_sector_cached(vol, lba, buffer)` checks for LBA matches (`hits++`), fetches from disk on miss (`misses++`), and evicts least-frequently-used entries (`evictions++`).
- **Flush & Cleanup:** `ntfs_cache_flush()` invalidates entries on volume unmount.

---

## 11. Error & Compression Model

- Requests for compressed (`is_compressed`) or encrypted (`is_encrypted`) streams return explicit unsupported error code (`-1`).
- Byte arithmetic overflow attempts are detected and safely rejected (`-1`).

---

## 12. Safety Invariants

1. **EOF Clamping:** Reads never return bytes beyond `data_size`.
2. **Buffer Safety:** Copies are strictly bounded by caller destination buffer size.
3. **Partition Bounds:** Sector reads check device `sector_count` bounds.
4. **Memory Lifetime:** `NTFS_File` retains an active reference to `NTFS_FileRecord` during its lifetime.

---

## 13. Important Files & APIs

| File Path | Description |
| :--- | :--- |
| [kernel/vfs/vfs_legacy/fs/ntfs/include/ntfs.h](file:///d:/Signatures_OS/kernel/vfs/vfs_legacy/fs/ntfs/include/ntfs.h) | `NTFS_File`, `NTFS_ReadCache`, and Phase 4 API function declarations. |
| [kernel/vfs/vfs_legacy/fs/ntfs/src/ntfs.c](file:///d:/Signatures_OS/kernel/vfs/vfs_legacy/fs/ntfs/src/ntfs.c) | Phase 4 implementation (`ntfs_file_open_by_record`, `ntfs_file_close`, `ntfs_file_read`, `ntfs_read_sector_cached`). |
| [kernel/vfs/vfs_legacy/fs/ntfs/src/ntfs_test.c](file:///d:/Signatures_OS/kernel/vfs/vfs_legacy/fs/ntfs/src/ntfs_test.c) | Test suite containing 60 unit, resident, non-resident, range, sparse, fragmented, initialized-size, and cache certification tests. |

---

## 14. Certification Tests & Results

60 kernel runtime certification tests were executed inside QEMU:

| Test Range | Category | Description | Status |
| :--- | :--- | :--- | :--- |
| **TESTS 1–14** | Phase 1 Foundation | Boot sector parsing, BPB validation, 12 corruption cases | **PASS** |
| **TESTS 15–27** | Phase 2 MFT Core | Record parsing, USA fixup, 8 corruption cases, mirror fallback | **PASS** |
| **TESTS 28–42** | Phase 3 Attribute Engine | Resident/non-resident attributes, data-runs, sparse, extent map, bootstrap | **PASS** |
| **TEST 43** | File Open by Record | Open record 0 by MFT number (`ntfs_file_open_by_record`) | **PASS** |
| **TEST 44** | Unnamed Stream Selection | Default open selects unnamed `$DATA` stream | **PASS** |
| **TEST 45** | Resident Full Read | Reads entire 16-byte resident payload | **PASS** |
| **TEST 46** | Resident Range Read | Reads offset range ("DATA") within resident stream | **PASS** |
| **TEST 47** | Resident EOF Clamping | Requested 50 bytes clamped to remaining 6 bytes | **PASS** |
| **TEST 48** | Resident Read at EOF | Read at offset equal to `data_size` returns 0 | **PASS** |
| **TEST 49** | Non-Resident Full Read | Reads 1024-byte non-resident cluster stream | **PASS** |
| **TEST 50** | Non-Resident Range Read | Reads 250 bytes across sector boundary at unaligned offset 15 | **PASS** |
| **TEST 51** | Non-Resident EOF Clamping | Requested 100 bytes clamped to remaining 6 bytes | **PASS** |
| **TEST 52** | Fragmented Read | Reads 40KB spanning non-contiguous physical LCN extents | **PASS** |
| **TEST 53** | Sparse Zero Synthesis | Reads sparse extent (VCN 4..13) and verifies 100% zero synthesis without I/O | **PASS** |
| **TEST 54** | Initialized Size Protection | Reads uninitialized gap (`offset > initialized_size`) and verifies 100% zero synthesis | **PASS** |
| **TEST 55** | Compressed Stream Rejection | Compressed `$DATA` stream rejected with `-1` | **PASS** |
| **TEST 56** | Read Cache Hits/Misses | Verifies cache hit and miss counters | **PASS** |
| **TEST 57** | Read Cache LRU Eviction | Verifies 64-entry LRU cache evictions | **PASS** |
| **TEST 58** | Read Cache Flush | Verifies cache invalidation on unmount | **PASS** |
| **TEST 59** | Memory Leak Audit | 10 open/read/close cycles completed with 0 leaks | **PASS** |
| **TEST 60** | Full Pipeline Integration | Mount -> Record Open -> Resident/Non-Resident/Sparse Reads -> Close -> Unmount | **PASS** |

**Observed Kernel Output:**
```
=========================================
 [NTFS PHASE 1, 2, 3 & 4 CERTIFICATION RESULTS]
   Total Tests Run : 60
   Passed          : 60
   Failed          : 0
 OVERALL STATUS     : PASS (100% CERTIFIED)
=========================================
```

---

## 15. Real NTFS Image Validation
- Phase 4 synthetic certification PASS; real NTFS image validation pending.

---

## 16. Known Limitations

- **Phase 5 Directory Index Engine:** Path resolution (`/foo/bar.txt` to MFT record) belongs to Phase 5.
- **Phase 8 File Writes:** File writing, allocation, and MFT modifications belong to future write phases.

---

## 17. Phase 5 Handoff (Directory & Index Engine)

Phase 5 can safely rely on the following Phase 4 contracts:
1. `ntfs_file_open_by_record(vol, record_num)` opens any validated MFT record.
2. `ntfs_file_read(file, offset, buffer, len)` reads file data, `$INDEX_ROOT`, or `$INDEX_ALLOCATION` streams across resident, non-resident, fragmented, or sparse extents.
3. `ntfs_file_close(file)` cleans up open file handles safely.
