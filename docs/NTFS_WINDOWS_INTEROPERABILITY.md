# ATOMS OS — Windows 11 NTFS Interoperability & BugCheck 0x24 Defense
**Document ID:** `NTFS-WIN-INTEROP-V1.0`  
**Target:** Microsoft `ntfs.sys` Kernel Internals, BugCheck 0x24 Prevention, and Clean Mounting  
**Hardware Target:** ASUS PRIME B750M-K (Intel Core i3-14100F, WD Blue SN5000 NVMe SSD)  
**Authors:** Developer A (Architecture) & Developer B (Forensics)  
**Date:** 2026-09-04  

---

## 1. Microsoft ntfs.sys Mount & Traversal Flow

When Windows 11 boots:
1. `winload.efi` loads the kernel (`ntoskrnl.exe`) and boot drivers, including `ntfs.sys`.
2. `ntfs.sys!NtfsMountVolume()` performs phase-1 read-only verification:
   - Reads Partition Boot Sector (VBR) at LBA 0; validates BPB geometry.
   - Reads Record 0 (`$MFT`); verifies `'FILE'` signature, USA fixups, and basic attributes.
   - Reads Record 3 (`$Volume`); checks `VOLUME_DIRTY` flag (`0x0001`).
3. If dirty flag is set: Windows defers normal boot and schedules `autochk.exe`.
4. If clean: `ntfs.sys` opens the root directory (Record 5) to locate `\Windows\System32\ntoskrnl.exe` and system registry hives.
5. In `ntfs.sys!NtfsFindIndexEntry()`:
   - Executes binary search over Record 5's `$INDEX_ROOT`.
   - Compares search keys against resident index entries using internal collation routines (`NtfsCollateNames`).
   - If an out-of-order entry or malformed router pointer is detected:
     $$\text{ntfs.sys!NtfsBugCheckDefinition() } \longrightarrow \text{ KeBugCheckEx(0x24, ...)}$$

---

## 2. Exhaustive BugCheck 0x24 Trigger Catalog

Microsoft's `ntfs.sys` raises `BugCheck 0x00000024 (NTFS_FILE_SYSTEM)` whenever an internal consistency invariant is violated:

### Trigger 1: `NtfsCheckBitmap()` Assertion Failure
- **Condition:** An MFT record has `flags & 0x0001` (`NTFS_FILE_IN_USE`), but the corresponding bit in `$MFT::$BITMAP` (Record 0) is `0` (`FREE`).
- **Mechanism:** `ntfs.sys` verifies consistency between the MFT record header and the allocation bitmap during file table validation. If a record claims to be in use while the allocator marks it free, the kernel panics to prevent cross-allocation data corruption.
- **ATOMS Defense:** Ensure bit $R$ is written to 1 in Record 0's `$BITMAP` and flushed **before** linking the file into the directory tree.

### Trigger 2: `NtfsFindIndexEntry()` Collation Violation
- **Condition:** In an index node, $Key_{i} > Key_{i+1}$ according to Unicode `$UpCase` comparison.
- **Mechanism:** The binary search algorithm assumes monotonic ordering. If a traversal detects an inverted key sequence, it declares the index corrupted.
- **ATOMS Defense:** Strictly locate insertion offset using `ntfs_collate_filenames()` with volume `$UpCase` table.

### Trigger 3: `NtfsCheckIndex()` Router Node Flag Violation
- **Condition:** In a directory with `$INDEX_ALLOCATION`, an entry in `$INDEX_ROOT` has `flags & 0x01 == 0` (leaf flag without child VCN).
- **Mechanism:** In multi-tier trees, all non-leaf entries must carry child VCN downlinks to route traversals. A leaf entry inside a router node violates B-Tree invariants.
- **ATOMS Defense:** If directory has `$INDEX_ALLOCATION`, descend to leaf `"INDX"` blocks for entry insertion; never insert leaf format entries into `$INDEX_ROOT`.

### Trigger 4: Unsorted Attribute Headers
- **Condition:** In an MFT record, an attribute of type $T_{i}$ is followed by an attribute of type $T_{j}$ where $T_i \ge T_j$.
- **Mechanism:** Attributes within an MFT record must be sorted strictly by numerical type code ($0x10 < 0x20 < 0x30 < 0x80 < 0x90 < 0xB0 < 0xFFFFFFFF$).
- **ATOMS Defense:** Insert attributes strictly in ascending type-code sequence.

---

## 3. Clean Volume Guarantees for Windows Boot

To guarantee seamless, transparent cross-booting into Windows 11 without triggering `chkdsk` or startup repair:
1. **Never Dirty the Volume Flag:** Do not set bit `0x0001` in `$Volume` attribute `0x70` unless an uncommitted journal transaction is in flight.
2. **Synchronize All Sizes:** Ensure logical size, allocated size, and directory entry sizes match exactly.
3. **Valid USA Fixups:** Every written sector must have valid update sequence numbers matching its sector trailers.
4. **Hardware FLUSH:** Always issue `NVMe FLUSH` before unmounting to ensure write buffers are committed to persistent NAND flash.
