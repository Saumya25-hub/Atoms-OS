# ATOMS OS — NTFS Phase 2: MFT Core Engine Architecture & Certification Document

This document records the design, implementation, trust boundaries, safety invariants, test evidence, and Phase 3 handoff specification for **NTFS Phase 2 — MFT Core Engine** in ATOMS OS.

---

## 1. Phase Status

| Sub-Phase | Description | Status |
| :--- | :--- | :--- |
| **2A** | `$MFT` Discovery | **COMPLETE** |
| **2B** | FILE Record Parser | **COMPLETE** |
| **2C** | USA / Fixup Handling | **COMPLETE** |
| **2D** | FILE Record Validation | **COMPLETE** |
| **2E** | `$MFTMirr` Fallback | **COMPLETE** |
| **2F** | Corruption Guards & Forensic Diagnostics | **COMPLETE** |

**Overall Phase 2 Status:** **PASS (100% CERTIFIED)**

---

## 2. What Was Implemented

Phase 2 builds directly upon the Phase 1 Volume Foundation to provide safe, verified MFT record processing:
- **`$MFT` Bootstrap & Record Read:** Computes exact sector LBA offsets from Phase 1 `$MFT` base LCNs and reads multi-sector FILE records safely through `BlockDevice` abstractions.
- **FILE Record Header Parser:** Parses on-disk `NTFS_FileRecordHeader` (48 bytes) into structured in-memory `NTFS_FileRecord` objects.
- **USA / Fixup Engine:** Complete implementation of NTFS Multi-Sector Transfer Protection. Verifies Update Sequence Numbers across all protected sectors and restores original 16-bit trailer words in-place.
- **Structural Validation:** Validates `"FILE"` magic signature, `usa_offset`, `usa_count`, `first_attribute_offset`, `bytes_in_use`, and `bytes_allocated` against record size limits.
- **Controlled `$MFTMirr` Fallback:** If reading or validating critical initial MFT records (Records 0–3) fails on primary `$MFT`, a fallback read is attempted on `$MFTMirr`.
- **Forensic Diagnostics:** Stage-by-stage diagnostic logging (`ntfs_mft_dump_diagnostics`) recording record state, header fields, source (Primary vs Mirror), and exact failure reasons.

---

## 3. Architecture Flow

```
[NTFS_VOLUME Context (Phase 1)]
            │
            ▼
[Calculated MFT Byte Offset (mft_byte_offset + record_num * record_size)]
            │
            ▼
[Bounded Sector Read via BlockDevice (ntfs_read_sector)]
            │
            ▼
[NTFS_RECORD_STATE_RAW (Owned Raw Record Buffer)]
            │
            ▼
[USA / Fixup Protection Engine (ntfs_mft_apply_fixup)]
            │
            ▼
[NTFS_RECORD_STATE_FIXUP_APPLIED (Sector Trailers Restored In-Place)]
            │
            ▼
[Header Structural Validation (ntfs_mft_validate_record)]
            │
            ▼
[NTFS_RECORD_STATE_VALIDATED (Trusted NTFS_FileRecord Output)]
```

If reading or validating primary critical MFT records (Records 0–3) fails:
```
Primary $MFT Failure -> Log Diagnostic -> $MFTMirr Fallback -> USA Fixup -> Validation -> Trusted Record (SRC_MIRROR)
```

---

## 4. Trust Boundary (`RAW → FIXUP → VALIDATED`)

To enforce security and integrity in kernel space:
1. **`NTFS_RECORD_STATE_RAW`:** Unprocessed disk bytes read into an owned heap buffer.
2. **`NTFS_RECORD_STATE_FIXUP_APPLIED`:** Sector trailers verified against expected USN and original 16-bit words restored.
3. **`NTFS_RECORD_STATE_VALIDATED`:** Record passed header magic (`"FILE"`), attribute bounds, and size checks. Only validated records can be returned to callers or passed to future phases.

Upper layers never access raw disk buffers directly.

---

## 5. Important Files

| File Path | Description |
| :--- | :--- |
| [kernel/vfs/vfs_legacy/fs/ntfs/include/ntfs.h](file:///d:/Signatures_OS/kernel/vfs/vfs_legacy/fs/ntfs/include/ntfs.h) | Added `NTFS_FileRecordHeader`, `NTFS_FileRecord`, trust state enums, and Phase 2 API declarations. |
| [kernel/vfs/vfs_legacy/fs/ntfs/src/ntfs.c](file:///d:/Signatures_OS/kernel/vfs/vfs_legacy/fs/ntfs/src/ntfs.c) | Phase 2 implementation (`ntfs_mft_apply_fixup`, `ntfs_mft_validate_record`, `ntfs_mft_read_record`, `ntfs_mft_dump_diagnostics`). |
| [kernel/vfs/vfs_legacy/fs/ntfs/src/ntfs_test.c](file:///d:/Signatures_OS/kernel/vfs/vfs_legacy/fs/ntfs/src/ntfs_test.c) | Extended test suite containing 27 unit, fixup corruption, header corruption, fallback, and boundary tests. |

---

## 6. Important Structures & APIs

### Key Structures
- `NTFS_FileRecordHeader`: 48-byte packed header layout matching on-disk structure.
- `NTFS_FileRecord`: In-memory parsed record containing:
  - `state` (`NTFS_RECORD_STATE_VALIDATED`), `source` (`PRIMARY` vs `MIRROR`), `record_number`, `lba_offset`, `record_size`.
  - Parsed fields: `usa_offset`, `usa_count`, `lsn`, `sequence_number`, `hard_link_count`, `first_attribute_offset`, `flags`, `bytes_in_use`, `bytes_allocated`, `base_file_record`.
  - `buffer`: Pointer to owned mutable 1024B (or configured size) record buffer.

### Key APIs
- `NTFS_FileRecord* ntfs_mft_read_record(NTFS_VOLUME* vol, uint32_t record_number)`: Reads, fixup-verifies, structurally validates, and returns an MFT record (or attempts mirror fallback for records 0–3).
- `void ntfs_mft_free_record(NTFS_FileRecord* record)`: Deallocates record and buffer.
- `bool ntfs_mft_apply_fixup(uint8_t* buffer, uint32_t record_size, uint32_t bytes_per_sector, const char** out_err)`: Applies Multi-Sector Transfer Protection fixup in-place.
- `bool ntfs_mft_validate_record(const uint8_t* buffer, uint32_t record_size, const char** out_err)`: Validates header fields.

---

## 7. USA / Fixup Contract

- **Algorithm:** For protected sector count $N = \text{record\_size} / \text{bytes\_per\_sector}$:
  1. Checks `usa_offset >= 0x24` and `usa_offset + (usa_count * 2) <= record_size`.
  2. Verifies `usa_count >= N + 1`.
  3. Extracts expected USN from `usa[0]`.
  4. Verifies trailers at `(i + 1) * bytes_per_sector - 2` against USN for all $i \in [0, N-1]$.
  5. If any trailer fails to match: **Fixup fails instantly**.
  6. Restores original replacement words from `usa[i+1]` to sector trailers in-place.
- **Failure Behavior:** A record failing fixup is rejected cleanly, never partially modified or marked validated.

---

## 8. `$MFTMirr` Fallback Scope

- **Scope:** Supported for critical initial MFT records (Records 0–3: `$MFT`, `$MFTMirr`, `$LogFile`, `$Volume`).
- **Behavior:** Triggered automatically if primary `$MFT` read, fixup, or validation fails.
- **Logging:** Primary failure reason is recorded in diagnostics before attempting fallback. If mirror succeeds, `rec->source` is set to `NTFS_RECORD_SRC_MIRROR`.

---

## 9. Safety Invariants

1. **Partition Bounds Safety:** `primary_lba + sectors_per_record` is checked against `vol->device->sector_count`. Out-of-bounds reads fail safely without disk access.
2. **Overflow Protection:** Record offset calculations (`record_number * record_size`) check for 64-bit integer multiplication overflow.
3. **Memory Safety:** Raw buffer memory allocated during failed reads or fixup failures is deallocated immediately.
4. **Header Invariants:** `bytes_in_use <= bytes_allocated <= record_size` and `first_attribute_offset >= usa_end`.

---

## 10. Tests & Runtime Evidence

27 kernel runtime certification tests were executed inside QEMU:

| Test Range | Category | Description | Status |
| :--- | :--- | :--- | :--- |
| **TESTS 1–14** | Phase 1 Foundation | Boot sector parsing, BPB validation, 12 corruption cases | **PASS** |
| **TEST 15** | Valid MFT Record | Read Record 0, USA fixup, trailer word restoration (0x1111, 0x2222) | **PASS** |
| **TESTS 16–19** | USA Fixup Corruptions | Sector 1 USN mismatch, Sector 2 USN mismatch, out-of-bounds USA offset, insufficient USA count | **PASS (All Rejected)** |
| **TESTS 20–23** | Header Corruptions | Bad magic (`"BAAD"`), attribute offset overlapping USA, `bytes_in_use > bytes_allocated`, `bytes_allocated > record_size` | **PASS (All Rejected)** |
| **TEST 24** | Mirror Fallback (Valid Primary) | Primary read succeeds; mirror not accessed | **PASS** |
| **TEST 25** | Mirror Fallback (Corrupt Primary) | Primary fails; fallback to `$MFTMirr` succeeds (`SRC_MIRROR`, trailer 0x3333) | **PASS** |
| **TEST 26** | Mirror Fallback (Both Corrupt) | Primary & Mirror fail; returns NULL safely | **PASS** |
| **TEST 27** | Partition Boundary Safety | Out-of-bounds record read (Record 999999) rejected safely | **PASS** |

**Observed Kernel Output:**
```
=========================================
 [NTFS PHASE 1 & 2 CERTIFICATION RESULTS]
   Total Tests Run : 27
   Passed          : 27
   Failed          : 0
 OVERALL STATUS     : PASS (100% CERTIFIED)
=========================================
```

---

## 11. Known Limitations

- **Phase 3 Non-Resident Runlists:** Phase 2 reads initial MFT records available at starting `$MFT` / `$MFTMirr` LCNs. Decoding non-resident `$DATA` runlists to access arbitrary non-contiguous MFT extents belongs to Phase 3.
- **Attribute Parsing:** Phase 2 inspects `first_attribute_offset` only for header bounds checking. Parsing attribute headers (`$STANDARD_INFORMATION`, `$FILE_NAME`, `$DATA`) belongs to Phase 3.

---

## 12. Phase 3 Handoff (Phase 3 — NTFS Attribute Engine)

Phase 3 can safely rely on the following Phase 2 invariants:
1. `ntfs_mft_read_record()` returns a fully validated `NTFS_FileRecord*` object with fixups applied and sector trailers restored.
2. `rec->buffer + rec->first_attribute_offset` marks the verified byte offset of the first attribute record.
3. `rec->bytes_in_use` guarantees the total valid byte length of headers and resident attributes within `rec->buffer`.
