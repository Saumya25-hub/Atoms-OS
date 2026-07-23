# ATOMS OS — NTFS Phase 3: Attribute Engine Architecture & Certification Document

This document records the design, implementation, data-run contracts, extent models, safety invariants, test evidence, and Phase 4 handoff specification for **NTFS Phase 3 — Attribute Engine** in ATOMS OS.

---

## 1. Phase Status

| Sub-Phase | Description | Status |
| :--- | :--- | :--- |
| **3A** | Resident Attributes | **COMPLETE** |
| **3B** | Non-Resident Attributes | **COMPLETE** |
| **3C** | Attribute Lists | **COMPLETE** |
| **3D** | Data-Run Decoding | **COMPLETE** |
| **3E** | Sparse Runs | **COMPLETE** |
| **3F** | Fragmented Files | **COMPLETE** |

**Overall Phase 3 Status:** **PASS (100% CERTIFIED)**

---

## 2. What Was Implemented

Phase 3 builds upon certified Phase 1 volume geometry and Phase 2 validated MFT records:
- **Resident & Non-Resident Attribute Parser:** Safe enumeration (`ntfs_attr_find`) of attribute headers starting at `first_attribute_offset` up to `bytes_in_use` / `0xFFFFFFFF` end marker. Parses common headers (`NTFS_AttributeHeader`), resident values, and non-resident metadata.
- **Data-Run Decoding Engine:** Variable-width byte decoder (`ntfs_decode_data_runs`) decoding run lengths and signed relative LCN deltas with proper sign extension.
- **Sparse Run Engine:** Detects sparse runs (`offset_bytes == 0`) and explicitly marks extents as `is_sparse = true`, `lcn_start = -1` without physical LCN movement or disk I/O.
- **Fragmented Extent Map (`NTFS_ExtentMap`):** Constructs logical VCN to physical LCN extent mappings (`NTFS_Extent`). Provides query API (`ntfs_extent_map_lookup`).
- **`$ATTRIBUTE_LIST` & `$MFT::$DATA` Bootstrap:** Parses `$ATTRIBUTE_LIST` entries (with max 256 iteration cycle protection). Automatically bootstraps Record 0 `$DATA` into an MFT extent map (`vol->mft_extent_map`) during volume mount.

---

## 3. Architecture Flow

```
[Validated NTFS_FileRecord (Phase 2)]
            │
            ▼
[Attribute Walker (ntfs_attr_find starting at first_attribute_offset)]
            │
            ▼
┌───────────────────────────┴───────────────────────────┐
│                                                       │
▼                                                       ▼
[Resident Attribute Parser]              [Non-Resident Attribute Parser]
│                                                       │
▼                                                       ▼
[Resident Value Extraction]              [Data-Run Mapping Pairs Decoder]
                                                        │
                                                        ▼
                                         [Signed LCN Delta & Sparse Engine]
                                                        │
                                                        ▼
                                         [NTFS_ExtentMap Construction]
                                                        │
                                                        ▼
                                         [VCN-to-LCN Lookup (ntfs_extent_map_lookup)]
```

---

## 4. Phase 1 & 2 Contracts Reused

- **Phase 1 Geometry (`NTFS_VOLUME`):** `bytes_per_sector`, `bytes_per_cluster`, `sectors_per_cluster`, `total_clusters`, `mft_lcn`.
- **Phase 2 MFT Core (`NTFS_FileRecord`):** Consumes validated records (`rec->state == NTFS_RECORD_STATE_VALIDATED`) with fixups already applied, sector trailers restored, and trusted `first_attribute_offset` and `bytes_in_use`.

---

## 5. Attribute Structures

### On-Disk Header Layouts
- `NTFS_AttributeHeader` (16 bytes packed): `type`, `length`, `non_resident`, `name_length`, `name_offset`, `flags`, `attribute_id`.
- `NTFS_ResidentAttributeHeader` (8 bytes packed): `value_length`, `value_offset`, `indexed_flag`.
- `NTFS_NonResidentAttributeHeader` (48 bytes packed): `starting_vcn`, `last_vcn`, `mapping_pairs_offset`, `allocated_size`, `data_size`, `initialized_size`.
- `NTFS_AttributeListEntry` (24 bytes packed): `type`, `length`, `name_length`, `name_offset`, `starting_vcn`, `base_file_reference`, `attribute_id`.

### Extent & Attribute Views
- `NTFS_Extent`: `vcn_start`, `cluster_count`, `lcn_start` (-1 if sparse), `is_sparse`.
- `NTFS_ExtentMap`: Array of `NTFS_Extent` entries, `extent_count`, `total_clusters`.
- `NTFS_Attribute`: In-memory metadata view with direct pointer into record buffer (`raw_attr_ptr`).

---

## 6. Data-Run Decoding Contract

1. Read header byte `header = runlist[pos++]`. End marker `0x00`.
2. `len_bytes = header & 0x0F`, `off_bytes = (header >> 4) & 0x0F`.
3. Decode `run_len` as unsigned little-endian integer ($1..8$ bytes).
4. Decode `lcn_delta` as signed little-endian integer ($0..8$ bytes). If top bit of top byte is set (`0x80`), sign-extend: `lcn_delta |= (-1LL << (off_bytes * 8))`.
5. Update `current_lcn += lcn_delta`. Verify `current_lcn >= 0`. Update `current_vcn += run_len`.

---

## 7. Sparse Extent Semantics

- **Trigger:** A data-run entry where `off_bytes == 0` (high nibble of header is 0).
- **Semantics:**
  - `is_sparse = true`
  - `lcn_start = -1`
  - `current_lcn` remains unchanged.
  - `current_vcn` advances by `run_len`.
  - Zero disk I/O occurs for sparse extents. Exposes enough info for Phase 4 to synthesize zero-filled reads.

---

## 8. Fragmentation Model

- Non-resident streams are represented by an ordered list of `NTFS_Extent` records.
- Signed relative LCN deltas allow backward physical placement on disk (`lcn_delta < 0`).
- Logical VCN continuity is strictly maintained (`ext[i+1].vcn_start == ext[i].vcn_start + ext[i].cluster_count`).
- `ntfs_extent_map_lookup(map, vcn, &out_extent)` resolves any VCN to its matching physical LCN or sparse state.

---

## 9. `$MFT::$DATA` Bootstrap

- During volume mount (`ntfs_mount`), Phase 3 reads Record 0 (`$MFT`).
- Finds the unnamed `$DATA` attribute (`0x80`).
- Decodes mapping pairs into `vol->mft_extent_map`.
- This extent map allows resolving MFT record locations even if `$MFT` itself is fragmented across non-contiguous clusters.

---

## 10. Memory Ownership

- `NTFS_Attribute` is a borrowed view pointing into the caller's validated `NTFS_FileRecord` buffer. The record must remain allocated while using `NTFS_Attribute`.
- `NTFS_ExtentMap` allocates an owned extent array via `kmalloc`. Callers must free it using `ntfs_extent_map_free()`.

---

## 11. Safety Invariants

1. **Attribute Bounds:** `offset + attr_length <= bytes_in_use`. Zero-length attribute records are rejected.
2. **Resident Value Bounds:** `value_offset + value_length <= attr_length`.
3. **Data-Run Bounds:** `pos + len_bytes + off_bytes <= runlist_len`.
4. **VCN Range Invariants:** `starting_vcn <= last_vcn`.
5. **Iteration Safeguard:** Attribute walking and attribute-list resolution are capped at 256 iterations to prevent infinite loops on corrupted media.

---

## 12. Important Files & APIs

| File Path | Description |
| :--- | :--- |
| [kernel/vfs/vfs_legacy/fs/ntfs/include/ntfs.h](file:///d:/Signatures_OS/kernel/vfs/vfs_legacy/fs/ntfs/include/ntfs.h) | Attribute headers, constants, `NTFS_Extent`, `NTFS_ExtentMap`, `NTFS_Attribute`, and Phase 3 APIs. |
| [kernel/vfs/vfs_legacy/fs/ntfs/src/ntfs.c](file:///d:/Signatures_OS/kernel/vfs/vfs_legacy/fs/ntfs/src/ntfs.c) | Phase 3 implementation (`ntfs_attr_find`, `ntfs_attr_get_resident_value`, `ntfs_decode_data_runs`, `ntfs_extent_map_lookup`, `ntfs_bootstrap_mft_extent_map`). |
| [kernel/vfs/vfs_legacy/fs/ntfs/src/ntfs_test.c](file:///d:/Signatures_OS/kernel/vfs/vfs_legacy/fs/ntfs/src/ntfs_test.c) | Extended test suite containing 42 unit, data-run, sparse, fragmentation, and attribute-list certification tests. |

---

## 13. Tests & Runtime Evidence

42 kernel runtime certification tests were executed inside QEMU:

| Test Range | Category | Description | Status |
| :--- | :--- | :--- | :--- |
| **TESTS 1–14** | Phase 1 Foundation | Boot sector parsing, BPB validation, 12 corruption cases | **PASS** |
| **TESTS 15–27** | Phase 2 MFT Core | Record parsing, USA fixup, 8 corruption cases, mirror fallback | **PASS** |
| **TEST 28** | Resident Attr Find | Find `$STANDARD_INFORMATION`, extract resident 24-byte payload | **PASS** |
| **TESTS 29–30** | Resident Attr Errors | Zero-length attribute rejection, out-of-bounds `value_offset` rejection | **PASS** |
| **TEST 31** | Non-Resident Attr Find | Parse non-resident `$DATA` metadata (VCN 0..15, 64KB data size) | **PASS** |
| **TEST 32** | Non-Resident Attr Error | `starting_vcn > last_vcn` rejected cleanly | **PASS** |
| **TEST 33** | Single Data-Run Decode | 16 clusters at LCN 4 decoded correctly | **PASS** |
| **TEST 34** | Multi-Run Signed LCN Deltas | Decoded LCN 100, LCN 90 (-10 delta), LCN 120 (+30 delta) | **PASS** |
| **TESTS 35–36** | Data-Run Errors | Truncated runlist rejection, zero run length rejection | **PASS** |
| **TEST 37** | Sparse Run Decoding | `off_bytes == 0` parsed as `is_sparse = true`, `lcn_start = -1` | **PASS** |
| **TESTS 38–39** | Extent Map Lookup | VCN lookup in fragmented map; out-of-bounds VCN 999 rejected | **PASS** |
| **TEST 40** | Attribute List Parsing | `NTFS_AttributeListEntry` parsing & base file ref extraction | **PASS** |
| **TEST 41** | `$MFT::$DATA` Bootstrap | Record 0 `$DATA` bootstrapped into `vol->mft_extent_map` | **PASS** |
| **TEST 42** | Full Pipeline Integration | Mount -> Record 0 Read -> Attr Walk -> Data-Run Decode -> Extent Map | **PASS** |

**Observed Kernel Output:**
```
=========================================
 [NTFS PHASE 1, 2 & 3 CERTIFICATION RESULTS]
   Total Tests Run : 42
   Passed          : 42
   Failed          : 0
 OVERALL STATUS     : PASS (100% CERTIFIED)
=========================================
```

---

## 14. Known Limitations

- **Phase 4 File Byte Reads:** Phase 3 provides extent maps (`NTFS_ExtentMap`) mapping logical VCNs to physical LCNs or sparse extents. General file byte reading (`ntfs_file_read`) belongs to Phase 4.
- **Phase 5 Directory Indexing:** B-Tree index root/allocation parsing for directory lookup belongs to Phase 5.

---

## 15. Phase 4 Handoff (Phase 4 — File Read Engine)

Phase 4 can safely rely on the following Phase 3 contracts:
1. `ntfs_attr_find()` finds resident and non-resident `$DATA` attributes inside validated MFT records.
2. `ntfs_attr_get_resident_value()` extracts resident file data safely.
3. `ntfs_decode_data_runs()` produces an `NTFS_ExtentMap` for non-resident files.
4. `ntfs_extent_map_lookup()` maps any file byte offset / VCN to physical disk LCN or identifies sparse extents.
