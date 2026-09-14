# ATOMS OS — NTFS Read/Write Engine Engineering Progress
**Author:** ATOMS OS Core Engineering & Filesystem Team  
**Platform:** ASUS PRIME B750M-K (Intel Core i3-14100F, WD Blue SN5000 NVMe M.2 500GB SSD)  
**Primary Target:** Pure Bare-Metal Hardware Execution via PXE (Zero QEMU)  

---

## Session 1: 2026-09-04 — Root Cause Discovery, Forensic Reconciliation & Engine Architecture

### 1. DATE
2026-09-04 19:50 UTC+5:30

### 2. OBJECTIVE
Build a real, robust, Windows-compatible native NTFS Read/Write engine for ATOMS OS on physical ASUS B750M-K hardware, resolving the root causes of the Windows 11 `NTFS_FILE_SYSTEM (0x24)` BugCheck.

### 3. AUDIT
- Non-destructive forensic read of on-disk Windows 11 NTFS volume (Partition 3, Start LBA 239616, 243 GB).
- Forensic inspection of Record 2766, Record 1454, Record 0 ($MFT::$BITMAP), and Record 5 (Root Directory B-tree).
- Discovered 5 MFT extents: Extent 0 at LCN `0xC0000` (VCN 0..5123), Extent 1 at LCN `0xEEEB65` (VCN 5123..10245).

### 4. FINDINGS
1. Physical Record 2766 contains `EM44C4~1.XML` (Flags: `0x0001` IN_USE, Sequence: `16`).
2. Record 0 `$BITMAP` bit 2766 is `0` (`FREE / UNALLOCATED`).
3. `ATOMS_WRITE_TEST.txt` record on disk is actually at Record 1454.
4. Record 5 (Root Directory) is a **Two-Tier B-Tree** (`$INDEX_ROOT` + `$INDEX_ALLOCATION`). Inside `$INDEX_ROOT`, `ATOMS_WRITE_TEST.txt` entry was present **out-of-order** before the end marker, while `EM44C4~1.XML` was down in `$INDEX_ALLOCATION`.

### 5. ROOT CAUSE
When ATOMS OS performed its earlier write:
1. It marked the MFT record as `IN_USE` on disk but failed to set and flush the corresponding bit in `$MFT::$BITMAP` (Record 0).
2. When Windows 11 booted, its allocator scanned `$MFT::$BITMAP`, observed bit 2766 was free, and legitimately reallocated Record 2766 for `EM44C4~1.XML`.
3. In Record 5, ATOMS appended an entry without Unicode collation sorting and inserted a leaf entry (`flags = 0`) into a router node (`$INDEX_ROOT`) of a two-tier directory. When `ntfs.sys` validated the root index, it detected out-of-order keys and crashed with `NTFS_FILE_SYSTEM (0x24)`.

### 6. REFERENCES
- Microsoft Open Specifications: `[MS-FSCC]` (File System Control Codes).
- Linux Kernel 6.x `fs/ntfs3`: `index.c` (B-tree traversal and leaf insertion), `bitmap.c` ($BITMAP allocation), `upcase.c` (Unicode collation), `inode.c` (MFT lifecycle).
- Microsoft NTFS Developer Notes & NTFS-3G.

### 7. IMPLEMENTATION
1. **Canonical Extent Mapping**: `ntfs_mft_record_to_physical_lba()` translates arbitrary record numbers to physical disk LBAs across all non-contiguous MFT extents.
2. **Atomic $MFT::$BITMAP Synchronization**: `ntfs_mft_set_record_allocated()` modifies the physical bitmap sector on disk and flushes.
3. **Sequence Number Lifecycle**: Reused records increment previous sequence number ($S_{old} + 1$).
4. **Two-Tier B-Tree Insertion**: `ntfs_btree_insert_ex()` traverses router keys down into `$INDEX_ALLOCATION`, inserts sorted leaf entries into 4096-byte `INDX` blocks, applies USA fixup, and leaves `$INDEX_ROOT` router keys intact.
5. **Unicode Collation Engine**: `ntfs_load_upcase_table()` loads 128KB `$UpCase` table from Record 10; `ntfs_collate_filenames()` performs two-pass case-insensitive collation with length and tie-breaking.
6. **Non-Resident Runlist Encoding**: `ntfs_create_file()` encodes non-resident data runs via `ntfs_encode_data_runs()`.
7. **Rollback Safety**: On any failure, frees clusters, clears bitmap bit, zeroes MFT record, and flushes.

### 8. FILES CHANGED
- `kernel/vfs/vfs_legacy/fs/ntfs/src/ntfs.c`: Core engine implementation.
- `kernel/vfs/vfs_legacy/fs/ntfs/include/ntfs.h`: Engine headers and API.
- `tools/atoms_control_center.py`: Synced ASUS B750M-K real hardware MAC `A0:AD:9F:C5:81:27`.
- `tools/autonomous_forensic_runner.py`: Synced ASUS B750M-K real hardware MAC `A0:AD:9F:C5:81:27`.
- `docs/NTFS_COMPLETE_GAP_MATRIX.md`: Complete subsystem gap audit.

### 9. TESTS
- Wake-On-LAN broadcast test to real ASUS hardware (`A0:AD:9F:C5:81:27`): PASS.
- Pure UEFI PXE network boot on ASUS PRIME B750M-K: PASS.
- Real hardware 1920x1080 framebuffer telemetry reception: PASS (5,925 chunks, 100% integrity).
- Non-destructive read-only forensic autopsy: PASS.

### 10. HARDWARE
- **Motherboard**: ASUS PRIME B750M-K (Intel B760/Haswell/RaptorLake LGA1700).
- **CPU**: Intel Core i3-14100F (8 logical cores online, BSP ID 0).
- **RAM**: 32 GB DDR4/DDR5 (32,421 MB usable).
- **Storage**: WD Blue SN5000 500GB NVMe SSD (`FW: 291000WD`, `Serial: 25211F806396`).
- **Network**: Realtek Gigabit Ethernet NIC (`A0:AD:9F:C5:81:27`).

### 11. FAILURES & FIXES
- **Issue**: Wake-On-LAN failed due to hardcoded stale MAC address (`0A:14:D6:E0:63:44`).
  - **Fix**: Updated both `autonomous_forensic_runner.py` and `atoms_control_center.py` to `A0:AD:9F:C5:81:27`. Tested and verified.
- **Issue**: Cache path in `ntfs_mft_read_record` assumed contiguous MFT.
  - **Fix**: Replaced with canonical `ntfs_mft_record_to_physical_lba()` lookup.

### 12. RESULT
Kernel compiled cleanly. Booted on bare metal. Telemetry validated. Subsystems aligned with NTFS specification.

### 13. COMMIT
- `16004f7`: `fix(tools): sync ASUS B750M-K real hardware MAC address A0:AD:9F:C5:81:27`
- `6c238b4`: `feat(ntfs): implement robust native NTFS write engine with two-tier B-tree leaf mutation, canonical extent mapping, collation, and $MFT::$BITMAP sync`

### 14. NEXT STEP
Execute Stage 1 controlled file creation test on real hardware and verify cross-boot interoperability with Windows 11.
