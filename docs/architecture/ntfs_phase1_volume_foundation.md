# ATOMS OS — NTFS Phase 1: Volume Foundation Architecture & Certification Document

This document records the design, implementation, safety invariants, test evidence, and Phase 2 handoff specification for **NTFS Phase 1 — Volume Foundation** in ATOMS OS.

---

## 1. Status

| Sub-Phase | Description | Status |
| :--- | :--- | :--- |
| **1A** | NTFS Signature Detection | **COMPLETE** |
| **1B** | NTFS Boot Sector Parsing | **COMPLETE** |
| **1C** | BPB and Boot Sector Validation | **COMPLETE** |
| **1D** | Sector and Cluster Geometry Engine | **COMPLETE** |
| **1E** | Volume Mount / Unmount Foundation | **COMPLETE** |
| **1F** | Forensic Diagnostics | **COMPLETE** |

**Overall Phase 1 Status:** **PASS (100% CERTIFIED)**

---

## 2. What Was Implemented

Phase 1 provides the production kernel foundation for mounting and identifying NTFS volumes:
- **Signature Detection:** Reliable detection of NTFS volumes via 8-byte `"NTFS    "` OEM identification and `0xAA55` boot sector magic signature over existing ATOMS storage abstractions.
- **Boot Sector Parser:** `#pragma pack(push, 1)` structure mapping all mandatory Phase-1-relevant BPB fields safely.
- **Validation Engine:** Strict 11-step BPB geometry, bounds, shift encoding, and arithmetic overflow protection. Malformed disks are rejected cleanly.
- **Geometry Engine:** Derivation of sector size, cluster size, volume cluster count, total byte capacity, `$MFT` and `$MFTMirr` starting LCN and byte offsets, FILE record size, and Index buffer size.
- **Lifecycle Engine:** Complete 7-stage mount sequence initializing an `NTFS_VOLUME` context and `VFS_Node` root. Clean unmount with resource cleanup.
- **Forensic Diagnostics:** Stage-by-stage diagnostic logging (`STAGE 1` through `STAGE 7`) reporting exact failure reasons during invalid media mounts.

---

## 3. Architecture Flow

The real implemented execution path for mounting an NTFS volume in ATOMS OS is:

```
[ATOMS Storage Driver (ATA)]
            │
            ▼
[Block Device Registry (block_device.c)]
            │
            ▼
[Logical Partition / Bounded Device (disk_manager.c)]  <-- Partition Boundaries Enforced Here
            │
            ▼
[NTFS Signature Detector & Boot Sector Read (ntfs.c)]
            │
            ▼
[NTFS Boot Sector Parser (ntfs.c)]
            │
            ▼
[BPB & Metadata Validation Engine (ntfs_validate_bpb)]
            │
            ▼
[Volume Geometry Engine (ntfs.c)]
            │
            ▼
[NTFS Volume Context & VFS Node Binds (ntfs_mount)]
```

---

## 4. Important Files

| File Path | Description |
| :--- | :--- |
| [kernel/vfs/vfs_legacy/fs/ntfs/include/ntfs.h](file:///d:/Signatures_OS/kernel/vfs/vfs_legacy/fs/ntfs/include/ntfs.h) | On-disk `NTFS_BootSector` definition, `NTFS_VOLUME` context, and Phase 1 API declarations. |
| [kernel/vfs/vfs_legacy/fs/ntfs/src/ntfs.c](file:///d:/Signatures_OS/kernel/vfs/vfs_legacy/fs/ntfs/src/ntfs.c) | Core Phase 1 implementation (detection, parsing, validation, geometry engine, mount/unmount, diagnostics). |
| [kernel/vfs/vfs_legacy/fs/ntfs/src/ntfs_test.c](file:///d:/Signatures_OS/kernel/vfs/vfs_legacy/fs/ntfs/src/ntfs_test.c) | Kernel runtime certification test suite testing valid geometry and 13 corrupted metadata rejection cases. |
| [kernel/kernel.c](file:///d:/Signatures_OS/kernel/kernel.c) | Kernel boot initialization calling `ntfs_init()` and `ntfs_run_tests()`. |
| [build.ps1](file:///d:/Signatures_OS/build.ps1) | Build system script compiling `ntfs.c` and `ntfs_test.c` into `kernel.bin`. |

---

## 5. Public / Internal Interfaces

### Structures
- `NTFS_BootSector`: 512-byte packed representation of on-disk boot sector.
- `NTFS_VOLUME`: Memory context storing validated volume parameters:
  - `bytes_per_sector`, `sectors_per_cluster`, `bytes_per_cluster`
  - `total_sectors`, `total_clusters`, `volume_size_bytes`
  - `mft_lcn`, `mft_byte_offset`, `mft_mirr_lcn`, `mft_mirr_byte_offset`
  - `file_record_size`, `index_buffer_size`, `volume_serial_number`

### Functions
- `void ntfs_init(void)`: Registers NTFS driver with VFS (`vfs_register_fs`).
- `VFS_Node* ntfs_mount(BlockDevice* device)`: Mounts NTFS volume on target block device. Returns `VFS_Node` root or `NULL`.
- `int ntfs_unmount(VFS_Node* mount_node)`: Unmounts volume and frees resources.
- `bool ntfs_validate_bpb(const NTFS_BootSector* bpb, uint64_t device_sector_count, const char** out_err_reason)`: Validates BPB integrity.
- `bool ntfs_decode_record_size(int8_t encoded, uint32_t bytes_per_cluster, uint32_t bytes_per_sector, uint32_t* out_size)`: Decodes FILE record and Index buffer size encodings.

---

## 6. Integration Points

- **Block Device Subsystem:** Reads sectors using `ntfs_read_sector()` which delegates to `BlockDevice` abstractions (`block_device_read()`).
- **Disk Manager:** Partition boundary safety is enforced by `disk_manager.c`. The `BlockDevice` passed to `ntfs_mount` represents a logical partition with relative LBA range `0..sector_count-1`.
- **VFS Layer:** Implements `FilesystemDriver` and binds root node to `VFS_Node` with `VFS_MOUNTPOINT` type.
- **Memory Allocation:** Dynamic allocation via `kmalloc`/`kfree` (`heap.h`).
- **Logging Subsystem:** Diagnostic logging via `display_print` (`display.h`), routed to kernel serial output (`qemu_doom_test.log`).

---

## 7. Safety Invariants

1. **Partition Boundary Safety:** All LBA reads check `lba + count <= dev->sector_count`. Out-of-bounds reads are aborted before reaching disk drivers.
2. **Corrupted Metadata Safety:** Disks with invalid OEM string, bad 0xAA55 signature, invalid sector sizes, non-power-of-2 cluster sizes, 0 sectors, or cluster size > 64KB are rejected at Stage 3/5 without panic, divide-by-zero, or memory corruption.
3. **LCN Boundary Enforcement:** `$MFT` and `$MFTMirr` starting clusters must be strictly less than `total_clusters`.
4. **Shift & Overflow Protection:** FILE record and Index buffer size decoding check shift counts (1..31) and multiplication bounds before computing record sizes.

---

## 8. Tests and Evidence

The kernel test suite (`ntfs_test.c`) was executed inside QEMU runtime:

| Test ID | Scenario | Expected Result | Observed Result | Status |
| :--- | :--- | :--- | :--- | :--- |
| **TEST 1** | Valid NTFS Volume Mount & Geometry | Successful Mount & Geometry | Geometry matched; cleanly unmounted | **PASS** |
| **TEST 2** | Record Size Decoding (-10, -12, +4) | 1024B, 4096B, 2048B | Exact match | **PASS** |
| **TEST 3** | Rejection: Wrong OEM Signature | Mount Rejected | Rejected at STAGE 3 | **PASS** |
| **TEST 4** | Rejection: Bad 0xAA55 Signature | Mount Rejected | Rejected at STAGE 3 | **PASS** |
| **TEST 5** | Rejection: Zero Bytes Per Sector | Mount Rejected | Rejected at STAGE 5 | **PASS** |
| **TEST 6** | Rejection: Invalid Sector Size (300) | Mount Rejected | Rejected at STAGE 5 | **PASS** |
| **TEST 7** | Rejection: Zero Sectors Per Cluster | Mount Rejected | Rejected at STAGE 5 | **PASS** |
| **TEST 8** | Rejection: Non-Power-of-2 Sectors/Cluster | Mount Rejected | Rejected at STAGE 5 | **PASS** |
| **TEST 9** | Rejection: Cluster Size Overflow (128KB) | Mount Rejected | Rejected at STAGE 5 | **PASS** |
| **TEST 10** | Rejection: Total Sectors > Device Bounds | Mount Rejected | Rejected at STAGE 5 | **PASS** |
| **TEST 11** | Rejection: $MFT LCN > Volume Clusters | Mount Rejected | Rejected at STAGE 5 | **PASS** |
| **TEST 12** | Rejection: $MFTMirr LCN > Volume Clusters | Mount Rejected | Rejected at STAGE 5 | **PASS** |
| **TEST 13** | Rejection: Invalid FILE Record Encoding (0) | Mount Rejected | Rejected at STAGE 5 | **PASS** |
| **TEST 14** | Rejection: Failed Block Read I/O | Mount Rejected | Rejected at STAGE 2 | **PASS** |

**Observed QEMU Log Output:**
```
=========================================
 [NTFS PHASE 1 CERTIFICATION RESULTS]
   Total Tests Run : 14
   Passed          : 14
   Failed          : 0
 OVERALL STATUS     : PASS (100% CERTIFIED)
=========================================
```

---

## 9. Known Limitations

The following features belong strictly to future phases and are intentionally NOT implemented in Phase 1:
- Phase 2: MFT record parsing, record headers, fixup sequence validation.
- Phase 3: Attribute header parsing (`$STANDARD_INFORMATION`, `$FILE_NAME`, `$DATA`).
- Phase 4: Non-resident data run parsing and file reading.
- Phase 5: B-tree index parsing and directory traversal.
- Phase 6+: Writes, allocations, recovery, or full VFS file operations.

---

## 10. Next Phase Handoff (Phase 2 — MFT Core Engine)

Phase 2 can safely rely on the following certified Phase 1 invariants:
1. `NTFS_VOLUME` context guarantees valid sector size (`bytes_per_sector`), cluster size (`bytes_per_cluster`), and total volume cluster count (`total_clusters`).
2. `$MFT` starting cluster (`mft_lcn`) and byte offset (`mft_byte_offset`) are guaranteed to be within valid volume cluster boundaries.
3. FILE record size (`file_record_size`) is guaranteed to be a decoded power-of-two value between 256 and 65536 bytes (typically 1024 bytes).
4. `ntfs_mount` provides a valid `VFS_Node` root bound to an initialized `NTFS_VOLUME` private data context.
