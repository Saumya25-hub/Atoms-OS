# ATOMS OS — NTFS Phase 5: Directory & Index Engine Architecture & Certification Document

This document records the design, implementation, index structure models, B+Tree traversal algorithms, `$INDEX_ROOT` / `$INDEX_ALLOCATION` / `$BITMAP` integration, directory enumeration, path resolution, test evidence, and Phase 6 handoff specification for **NTFS Phase 5 — Directory & Index Engine** in ATOMS OS.

---

## 1. Phase Status

| Sub-Phase | Description | Status |
| :--- | :--- | :--- |
| **5A** | `$INDEX_ROOT` Parsing | **COMPLETE** |
| **5B** | `$INDEX_ALLOCATION` & INDX Validation | **COMPLETE** |
| **5C** | `$BITMAP` Allocation Check | **COMPLETE** |
| **5D** | B+Tree Directory Traversal | **COMPLETE** |
| **5E** | Directory Enumeration | **COMPLETE** |
| **5F** | Path Resolver & Phase 4 Read Integration | **COMPLETE** |

**Overall Phase 5 Status:** **PASS (100% CERTIFIED)**

---

## 2. Directory Index Architecture

```
[Path String: "/System/Apps/Test.txt"]
            │
            ▼
[ntfs_resolve_path()] ──► Tokenize Path Components: ["System", "Apps", "Test.txt"]
            │
            ▼
[Root MFT Record 5]
            │
            ▼
┌───────────────────────────┴───────────────────────────┐
│                                                       │
▼                                                       ▼
[Parse $INDEX_ROOT:$I30 (5A)]            [Parse $INDEX_ALLOCATION:$I30 (5B)]
│                                                       │
│                                                       ▼
│                                         [Check $BITMAP Allocation (5C)]
│                                                       │
│                                                       ▼
│                                         [Validate INDX Magic & Apply Fixup]
│                                                       │
│                                                       ▼
│                                         [Traverse B+Tree Nodes (5D)]
└───────────────────────────────────────┬───────────────┘
                                        │
                                        ▼
                           [Extract Target MFT File Ref]
                           (lower 48 bits = Record Number)
                                        │
                                        ▼
                        [ntfs_open_file_by_path()]
                        (Calls Phase 4 ntfs_file_open_by_record)
                                        │
                                        ▼
                        [ntfs_file_read() -> Byte Content]
```

---

## 3. Phase 1–4 Contracts Reused

- **Phase 1 Geometry (`NTFS_VOLUME`):** `bytes_per_sector`, `bytes_per_cluster`, `index_buffer_size`.
- **Phase 2 MFT Core:** MFT record reading (`ntfs_mft_read_record`) and USA sector trailer fixup (`ntfs_mft_apply_fixup`).
- **Phase 3 Attribute Engine:** Attribute lookup (`ntfs_attr_find`) for `$INDEX_ROOT` (`0x90`), `$INDEX_ALLOCATION` (`0xA0`), `$BITMAP` (`0xB0`), and resident value extraction (`ntfs_attr_get_resident_value`).
- **Phase 4 File Read Engine:** File handles (`NTFS_File`, `ntfs_file_open_by_record`, `ntfs_file_read`, `ntfs_file_close`).

---

## 4. `$INDEX_ROOT` Parsing Model (5A)

- Resident attribute `0x90` (`$INDEX_ROOT`, name `$I30`).
- Contains `NTFS_IndexRootHeader` and `NTFS_IndexHeader`.
- Iterates over `NTFS_IndexEntry` structures up to end-marker `NTFS_INDEX_ENTRY_LAST` (`0x02`).
- Extracts target MFT file reference (`file_reference & 0xFFFFFFFFFFFFULL`), sequence number (`file_reference >> 48`), and embedded `$FILE_NAME` attribute keys.

---

## 5. `$INDEX_ALLOCATION` & INDX Validation Model (5B)

- Non-resident attribute `0xA0` (`$INDEX_ALLOCATION`, name `$I30`).
- Consists of $4096$-byte index allocation blocks headed by `NTFS_IndexBlockHeader` ("INDX" magic).
- `ntfs_indx_validate_and_fixup()` verifies `"INDX"` magic and applies sector trailer USA fixups before trusting index entries.

---

## 6. `$BITMAP` Semantics (5C)

- Attribute `0xB0` (`$BITMAP`, name `$I30`).
- `ntfs_index_bitmap_is_allocated(vol, dir_rec, vcn)` verifies whether an `$INDEX_ALLOCATION` block is active before reading. Absence of `$BITMAP` defaults to active.

---

## 7. B+Tree Traversal Architecture (5D)

- `ntfs_dir_lookup_entry()` searches ordered index entries:
  1. Inspects `$INDEX_ROOT:$I30` entries.
  2. Compares target string against `$FILE_NAME` key.
  3. If match found: returns target MFT reference.
  4. If entry has child node (`NTFS_INDEX_ENTRY_HAS_SUBNODE`): extracts child VCN and follows into `$INDEX_ALLOCATION` INDX block.
- Enforces a 32-level depth limit to prevent infinite recursion on corrupted media.

---

## 8. Filename / UTF-16 Comparison Semantics

- `ntfs_filename_cmp()` performs case-insensitive comparison between ASCII target strings and UTF-16 `$FILE_NAME` keys without requiring null termination.

---

## 9. Directory Enumeration Architecture (5E)

- `ntfs_dir_enum()` enumerates all items in a directory.
- Collects entries from `$INDEX_ROOT` and `$INDEX_ALLOCATION`.
- Suppresses DOS 8.3 alias duplicates (namespace 2) in favor of Win32 / Win32+DOS names.

---

## 10. Path Resolver Architecture (5F)

- `ntfs_resolve_path(vol, path, &out_rec)`:
  - Root path `"/"` resolves to MFT Record 5.
  - Tokenizes path (`/System/Apps/Test.txt`).
  - Resolves each component sequentially via parent directory B+Tree index lookups.
  - Enforces directory FILE flags (`rec->flags & NTFS_FILE_DIRECTORY`) on intermediate components.
- `ntfs_open_file_by_path(vol, path)`:
  - Connects `ntfs_resolve_path()` directly to Phase 4 `ntfs_file_open_by_record()`.

---

## 11. Safety Invariants & Traversal Limits

1. **Depth Limit:** B+Tree traversal capped at 32 levels.
2. **Bounds Checking:** Entry length, key length, and header offsets checked before dereferencing.
3. **Directory Flag Guard:** Intermediate components must be validated directory records.
4. **End-Marker Safety:** Traversal halts immediately on `NTFS_INDEX_ENTRY_LAST`.

---

## 12. Important Files & APIs

| File Path | Description |
| :--- | :--- |
| [kernel/vfs/vfs_legacy/fs/ntfs/include/ntfs.h](file:///d:/Signatures_OS/kernel/vfs/vfs_legacy/fs/ntfs/include/ntfs.h) | Index headers, `NTFS_DirEntry`, and Phase 5 API function declarations. |
| [kernel/vfs/vfs_legacy/fs/ntfs/src/ntfs.c](file:///d:/Signatures_OS/kernel/vfs/vfs_legacy/fs/ntfs/src/ntfs.c) | Phase 5 implementation (`ntfs_dir_lookup_entry`, `ntfs_dir_enum`, `ntfs_resolve_path`, `ntfs_open_file_by_path`). |
| [kernel/vfs/vfs_legacy/fs/ntfs/src/ntfs_test.c](file:///d:/Signatures_OS/kernel/vfs/vfs_legacy/fs/ntfs/src/ntfs_test.c) | Test suite containing 78 unit, corruption, B+Tree, enumeration, path resolution, and end-to-end integration tests. |

---

## 13. Certification Tests & Results

78 kernel runtime certification tests were executed inside QEMU:

| Test Range | Category | Description | Status |
| :--- | :--- | :--- | :--- |
| **TESTS 1–14** | Phase 1 Foundation | Boot sector parsing, BPB validation, 12 corruption cases | **PASS** |
| **TESTS 15–27** | Phase 2 MFT Core | Record parsing, USA fixup, 8 corruption cases, mirror fallback | **PASS** |
| **TESTS 28–42** | Phase 3 Attribute Engine | Resident/non-resident attributes, data-runs, sparse, extent map, bootstrap | **PASS** |
| **TESTS 43–60** | Phase 4 Read Engine | Resident/non-resident reads, range, sparse zeroing, initialized size, read cache | **PASS** |
| **TEST 61** | `$INDEX_ROOT` Parsing | Parses `$I30` root header and filename key | **PASS** |
| **TEST 62** | UTF-16 Filename Cmp | Case-insensitive UTF-16 / ASCII matching (`ntfs_filename_cmp`) | **PASS** |
| **TEST 63** | End Marker Enforcement | Iteration stops cleanly on `NTFS_INDEX_ENTRY_LAST` | **PASS** |
| **TEST 64** | Malformed Entry Error | Zero entry length rejected safely | **PASS** |
| **TEST 65** | `$INDEX_ALLOCATION` Parsing | Parses `INDX` block and applies USA fixup | **PASS** |
| **TEST 66** | Bad INDX Magic Error | Invalid magic ('BADX') rejected safely | **PASS** |
| **TEST 67** | INDX USA Mismatch Error | USA trailer mismatch rejected safely | **PASS** |
| **TEST 68** | `$BITMAP` Allocation Check | `ntfs_index_bitmap_is_allocated` verifies active bit | **PASS** |
| **TEST 69** | `$BITMAP` Bounds Safety | Out-of-bounds bit lookup handled safely | **PASS** |
| **TEST 70** | `$INDEX_ROOT` Lookup | B+Tree lookup resolves child in `$INDEX_ROOT` | **PASS** |
| **TEST 71** | `$INDEX_ALLOCATION` Lookup | B+Tree lookup resolves child in `$INDEX_ALLOCATION` | **PASS** |
| **TEST 72** | Lookup Not-Found | Missing filename component returns false | **PASS** |
| **TEST 73** | B+Tree Cycle Guard | 32-level recursion depth limit verified | **PASS** |
| **TEST 74** | Directory Enumeration | `ntfs_dir_enum` returns item array | **PASS** |
| **TEST 75** | DOS Alias Suppression | Suppresses DOS namespace duplicates | **PASS** |
| **TEST 76** | Path Resolver Root | Path `"/"` resolves to Record 5 | **PASS** |
| **TEST 77** | Multi-Level Path Resolver | Path `"/System/Apps/Test.txt"` resolves to Record 8 | **PASS** |
| **TEST 78** | Full End-to-End Pipeline | Path `"/System/Apps/Test.txt"` -> Record 8 -> `ntfs_open_file_by_path` -> `ntfs_file_read` verifies payload `"PHASE5_END_TO_END_INTEGRATION_OK"` | **PASS** |

**Observed Kernel Output:**
```
=========================================
 [NTFS PHASE 1, 2, 3, 4 & 5 CERTIFICATION RESULTS]
   Total Tests Run : 78
   Passed          : 78
   Failed          : 0
 OVERALL STATUS     : PASS (100% CERTIFIED)
=========================================
```

---

## 14. Real NTFS Image Validation
- Phase 5 synthetic certification PASS; real NTFS image validation pending.

---

## 15. Known Limitations

- **Phase 6 VFS Driver Production Mount:** Registering NTFS as production VFS root driver belongs to Phase 6.
- **Phase 8 File Writes:** File writing, allocation, and directory creation belong to future write phases.

---

## 16. Phase 6 Handoff (VFS Driver Production Mount)

Phase 6 can safely rely on the following Phase 5 contracts:
1. `ntfs_resolve_path(vol, path, &rec_num)` resolves any nested directory path to its target MFT record.
2. `ntfs_open_file_by_path(vol, path)` opens a file handle directly from a path string.
3. `ntfs_dir_enum(vol, dir_rec, &entries, &count)` enumerates directory contents for VFS `readdir` implementations.
