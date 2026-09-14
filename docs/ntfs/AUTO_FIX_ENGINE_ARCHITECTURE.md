# ATOMS OS — Safe Auto-Fix Engine Architecture Specification
**Document Classification:** ARCHITECTURAL DESIGN ONLY — DISABLED BY DEFAULT  
**Operating Safety State:** PASSIVE / READ-ONLY AUDIT MODE  
**Execution Interlock:** REQUIRES EXPLICIT USER APPROVAL TOKEN BEFORE MUTATION  

---

## 1. Architectural Philosophy & Safety Interlocks

The Auto-Fix Engine is designed as a safety-critical recovery tool for NTFS metadata restoration. It operates under strict aeronautical-grade invariants:

```
[Default State: DISABLED]
          ↓
[Forensic Proof Produced]
          ↓
[Repair Manifest Generated]
          ↓
[Pre-Mutation Backup Verified]
          ↓
[User Explicit Approval Received]
          ↓
[Atomic Surgical Execution]
          ↓
[Read-Back Verification]
          ↓
[Lock Re-Engaged (DISABLED)]
```

### Absolute Rules:
1. **Disabled by Default:** The engine contains a hard hardware/software interlock preventing execution unless an explicit override flag is passed.
2. **Zero Heuristics:** No speculative repairs. Repairs are only permitted for exact, mathematically proven inverse transformations.
3. **Atomic Rollback Buffer:** Before modifying even a single sector, the original target sectors must be captured into a non-volatile rollback buffer.

---

## 2. Standard Repair Manifest Requirements

Every proposed surgical repair operation **MUST** be defined in a formal Repair Manifest containing the following 11 mandatory fields:

1. **BUG:** Explicit description of the identified flaw.
2. **EVIDENCE:** Exact hexadecimal dumps, memory traces, or logs demonstrating the defect.
3. **AFFECTED STRUCTURE:** Name and MFT record / cluster of target metadata.
4. **EXPECTED PRE-STATE:** Exact byte values currently on disk.
5. **EXPECTED POST-STATE:** Exact byte values to be written to disk.
6. **EXACT SECTORS:** Physical LBA, sector count, partition relative offset.
7. **EXACT BYTES / STRUCTURES:** Offsets within sector, struct fields affected.
8. **REVERSIBILITY:** Proof that the operation can be inverted back to the pre-state.
9. **ROLLBACK PLAN:** Byte-level inverse patch ready for immediate deployment.
10. **VALIDATION PLAN:** Specific read-back tests verifying both NTFS consistency and zero side-effects.
11. **WINDOWS-COMPATIBILITY ARGUMENT:** Proof that the resulting on-disk structure satisfies `ntfs.sys` validation checks.

---

## 3. Surgical Repair Candidate: Root Directory (Record 5) Restoration

### Candidate Manifest #001:

- **BUG:** Index entry `ATOMS_WRITE_TEST.txt` (120 bytes) is appended out of collation order in Record 5 `$INDEX_ROOT`, triggering Windows BSOD `0x24`.
- **EVIDENCE:** Forensic dump of Record 5 shows `abs_insert_pos` containing the entry before the End Marker, with headers incremented by 120 bytes.
- **AFFECTED STRUCTURE:** MFT Record 5 (Root Directory `$INDEX_ROOT`).
- **EXPECTED PRE-STATE (Current):**
  - Record 5 contains 120-byte `new_entry` at `abs_insert_pos`.
  - `idx_hdr->total_size`, `allocated_size`, `res_hdr->value_length`, `attr_hdr->length`, and `fhdr->bytes_in_use` are 120 bytes larger than original.
  - End marker is at `abs_insert_pos + 120`.
- **EXPECTED POST-STATE (Restored):**
  - Memory from `abs_insert_pos + 120` to `fhdr->bytes_in_use` shifted backward by 120 bytes using `memmove(rec_buf + abs_insert_pos, rec_buf + abs_insert_pos + 120, bytes_to_shift)`.
  - Slack space at tail zeroed out (`memset(rec_buf + fhdr->bytes_in_use - 120, 0, 120)`).
  - All 5 size fields decremented by 120 bytes:
    - `idx_hdr->total_size -= 120;`
    - `idx_hdr->allocated_size -= 120;`
    - `res_hdr->value_length -= 120;`
    - `attr_hdr->length -= 120;`
    - `fhdr->bytes_in_use -= 120;`
  - USA fixup recalculated: `usa[0]++`, trailers at 510 and 1022 updated.
- **EXACT SECTORS:**
  - Partition 3, Physical LBA: `(vol->mft_lcn * 8) + 10`.
  - Sector Count: 2 sectors (1024 bytes).
- **REVERSIBILITY:** 100% reversible.
- **ROLLBACK PLAN:** Re-inserting the 120-byte entry returns to current state.
- **VALIDATION PLAN:**
  1. `ntfs_dir_enum()` on Record 5 returns exactly 29 entries (the original pre-test set).
  2. `ntfs_dir_lookup_entry("ATOMS_WRITE_TEST.txt")` returns `false`.
  3. `bytes_in_use` matches pre-test value.
- **WINDOWS-COMPATIBILITY ARGUMENT:** Restores Record 5 to its identical, original Windows 11 B-tree state where all entries are sorted and valid.

---

## 4. Surgical Repair Candidate: Record 2766 Decommissioning

### Candidate Manifest #002:

- **BUG:** Record 2766 has `flags = NTFS_FILE_IN_USE` (`0x0001`), while Record 0 `$BITMAP` bit 2766 is `0`.
- **EVIDENCE:** MFT Record 2766 header dump vs Record 0 `$BITMAP` query.
- **AFFECTED STRUCTURE:** MFT Record 2766.
- **EXPECTED PRE-STATE (Current):** Formatted 1024-byte record with `FILE` magic, attributes, and payload.
- **EXPECTED POST-STATE (Restored):**
  - Option A (Safe Zeroing): Zero all 1024 bytes (`memset(rec_buf, 0, 1024)`), returning it to virgin uninitialized MFT space.
  - Option B (Deallocated Record): Clear `flags` (`hdr->flags = 0x0000`), increment sequence number (`hdr->sequence_number++`), set `bytes_in_use = 56`.
- **EXACT SECTORS:**
  - Partition 3, Physical LBA: Extent 1 LBA (Mapped via `vol->mft_extent_map`).
  - Sector Count: 2 sectors (1024 bytes).
- **REVERSIBILITY:** 100% reversible.
- **VALIDATION PLAN:** Probing Record 2766 verifies `is_virgin == true` or `flags == 0x0000`.
- **WINDOWS-COMPATIBILITY ARGUMENT:** Eliminates the inconsistency between Record 2766 and `$MFT::$BITMAP`. Both will agree that Record 2766 is unallocated.
