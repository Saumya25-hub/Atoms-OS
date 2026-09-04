# ATOMS OS — FORENSIC REPORT (TASK 1)
## Mission: BOFS Phase 13 — Real-Hardware Native BOFS Certification (Full Physical Storage + Real File + Persistence + Recovery + Stress)

**Protocol:** ATOMS OS Engineering Protocol V1 — RULE 0 (TASK 1 FORENSIC TEAM)  
**Date:** 2026-09-05  
**Baseline Checkpoint:** `PHASE13_PRECHECKPOINT = 1b472fdcb9a39a6bf129e1695d88a95c510cf959` (`1b472fd`)  
**Target Hardware Profile:** ASUS PRIME B750M-K (Intel Core i3-14100F, Haswell/RaptorLake x86_64, 32GB RAM, WD Blue SN5000 500GB NVMe SSD)  
**Status:** FORENSIC INVESTIGATION COMPLETE — NO CODE MODIFIED  

---

## 1. Executive Forensic Assessment

Phase 13 represents the final physical storage certification milestone of the BOFS filesystem engineering program. Phases 3 through 12 have certified all logical layers:
- Phase 3: Binary on-disk structures and geometry validation (`0x53464F42`).
- Phase 4: Block bitmap allocation, buddy/extent clustering, coalescing, zero leakage.
- Phase 5: Inode metadata, extent tree, direct/indirect mapping, byte-exact I/O.
- Phase 6: B+Tree directory topology, lexicographical ordering, UTF-8 preservation.
- Phase 7: DAC permissions (0600/0644/0755), UID/GID enforcement, zero-mutation denial.
- Phase 8: Write-Ahead Logging (WAL), atomic transactions, crash consistency, fail-closed recovery.
- Phase 9: VFS mount integration, dynamic node management, Ring 3 syscall gateway, user-pointer sanitization.
- Phase 10: BOSX native executable loading, W^X enforcement, independent address spaces.
- Phase 11: File Manager production UI binding, zero fake-content, readdir enumeration.
- Phase 12: 16-layer comprehensive forensic debug dashboard, First-Failure root cause engine, cross-layer timeline ring buffer, and bare-metal ASUS B750M-K proof.

### The Phase 13 Mission:
Transition BOFS from controlled/in-memory verification to an authoritative, dedicated physical storage volume with real persistence across hardware reboots, real physical file operations, real directory trees, real crash recovery, real physical stress, and strict zero-mutation guarantees for foreign physical storage (Windows NTFS, EFI, MSR, Recovery).

---

## 2. Inventory of Existing Storage & Filesystem Infrastructure

| Subsystem | File Location | Key Functions / Structs | Current State & Capability | Reusability in Phase 13 |
|---|---|---|---|---|
| **NVMe Driver** | `kernel/drivers/storage/nvme/nvme.c` | `nvme_init()`, `nvme_read()`, `nvme_write()`, `nvme_flush()` | Discovers PCI Mass Storage `0x01:0x08`, maps BAR0, establishes admin/IO queues, registers `nvme0n1`. | Production-grade; fully reusable for physical device probing. |
| **AHCI SATA Driver** | `kernel/drivers/storage/ahci/ahci.c` | `ahci_init()`, `ahci_port_read()`, `ahci_port_write()` | Discovers PCI Mass Storage `0x01:0x06`, initializes ports, registers `sda`, `sdb`, etc. | Production-grade; fully reusable for SATA drives. |
| **Partition Scanner** | `kernel/drivers/storage/partition/gpt.c` | `gpt_scan_device()`, `gpt_get_telemetry()`, `gpt_get_windows_ntfs_bdev()` | Parses GPT Header, validates GUIDs, registers sub-blockdevices (`nvme0n1p1`..`p5`), identifies NTFS and ESP. | Essential safety baseline; isolates foreign partitions. |
| **Block Device Layer**| `kernel/vfs/vfs_legacy/storage/include/block_device.h` | `block_device_register()`, `block_device_get()`, `block_device_read()`, `block_device_write()` | Abstract I/O dispatch interface with sector size, sector count, and `read_only` flag. | Reusable; provides uniform abstraction for physical and mock drives. |
| **BOFS Format Engine**| `kernel/vfs/bofs/include/bofs_format.h`, `kernel/vfs/bofs/src/bofs_validator.c` | `bofs_calc_geometry()`, `bofs_validate_superblock()`, `bofs_init_superblock()`, `bofs_crc32()` | Computes layout, validates geometry, serializes Superblock (`0x53464F42`) with CRC32. | Core format authority; ready for physical block formatting. |
| **BOFS Allocator** | `kernel/vfs/bofs/src/bofs_alloc.c` | `bofs_allocator_init()`, `bofs_alloc_block()`, `bofs_free_block()` | Manages in-memory and on-disk block/inode bitmaps. | Certified; ready for physical block device binding. |
| **BOFS File Engine** | `kernel/vfs/bofs/src/bofs_file.c` | `bofs_fs_init()`, `bofs_file_open()`, `bofs_file_read()`, `bofs_file_write()`, `bofs_file_truncate()` | Extent-based multi-block file I/O with checksums. | Certified; connects directly to VFS. |
| **BOFS Directory** | `kernel/vfs/bofs/src/bofs_dir.c` | `bofs_dir_lookup()`, `bofs_dir_insert()`, `bofs_dir_remove()`, `bofs_init_dir_node()` | B+Tree leaf/internal directory indexing. | Certified; drives hierarchical physical namespace. |
| **BOFS Security** | `kernel/vfs/bofs/src/bofs_security.c` | `bofs_sec_check_permission()`, `bofs_sec_create()` | DAC permissions evaluation (owner/group/other). | Certified; enforces file access boundaries. |
| **BOFS WAL Engine** | `kernel/vfs/bofs/src/bofs_wal.c` | `bofs_wal_init()`, `bofs_wal_format()`, `bofs_wal_mount()`, `bofs_wal_recover()` | Write-Ahead Log transaction lifecycle, crash replay. | Certified; guarantees on-disk transactional durability. |
| **BOFS VFS Adapter** | `kernel/vfs/bofs/src/bofs_vfs.c` | `bofs_vfs_mount_cb()`, `bofs_vfs_open_cb()`, `bofs_vfs_read_cb()`, `bofs_vfs_write_cb()` | Production VFS callback table (`bofs_fs_driver`). | Certified; serves as the production gateway. |
| **Forensic Dashboard**| `kernel/debug/bofs/bofs_forensic_dashboard.c` | `bofs_forensic_dashboard_run()`, `p12_draw_dashboard()`, `update_spinner()` | 2560x1600 / 1080p ABDE 4-panel truth machine with live UDP screenshot streaming. | Certified in Phase 12; provides authoritative visual interface for Phase 13. |

---

## 3. Physical Storage Architecture & Safety Gate Protocol

### A. Real Target Hardware Profile (ASUS PRIME B750M-K)
Telemetry and physical tests established the exact storage topography of the physical workstation:
- **PCI Storage Controller:** Western Digital WD Blue SN5000 500GB NVMe M.2 SSD (`0x15B7:0x5017`) at `PCI 02:00.0`.
- **Global BlockDevice:** `nvme0n1` ($976,773,168$ sectors = $465.76$ GiB).
- **Physical Partitions Present:**
  - Partition 1: Start LBA 2048, 100 MB [EFI System Partition / FAT32]
  - Partition 2: Start LBA 206848, 16 MB [Microsoft Reserved / MSR]
  - Partition 3: Start LBA 239616, 243 GB [Microsoft Basic Data / Windows 11 NTFS]
  - Partition 4: Windows Recovery Environment
  - Partition 5: OEM Diagnostics / Recovery

### B. The Absolute Invariant — Zero Foreign Storage Mutation
$$\text{FOREIGN STORAGE WRITES} = 0 \text{ BYTES (STRICTLY HARDWARE-ENFORCED)}$$

1. Every detected Windows NTFS, EFI System, MSR, or Recovery partition has its `BlockDevice.read_only` flag set to `true` at discovery time.
2. Any write request to an LBA belonging to a foreign partition is blocked at the lowest driver level and rejected with `-EPERM`.
3. Under no circumstances will ATOMS OS format or modify `nvme0n1p1`, `nvme0n1p2`, `nvme0n1p3`, `nvme0n1p4`, or `nvme0n1p5`.

### C. Human Safety Confirmation Gate (Section 2 & 3)
Before formatting any physical volume:
1. ATOMS OS scans for candidate storage devices and partitions.
2. Target candidate criteria:
   - Must NOT be an EFI System Partition.
   - Must NOT be an MSR partition.
   - Must NOT be a Windows Recovery partition.
   - Must NOT be a detected NTFS or FAT32 foreign user partition.
   - Must have capacity $\ge 32\text{ MB}$ (minimum BOFS format quantum).
   - Must be explicitly labeled or designated as `BOFS_TEST_VOLUME` or a dedicated secondary disk (e.g. Dedicated USB Drive / Dedicated Test Disk).
3. If no dedicated candidate is attached to the physical PC:
   $$\text{PHYSICAL BOFS STORAGE} = \text{NOT AVAILABLE (NO DEDICATED TEST VOLUME)}$$
   $$\text{PHASE 13 PHYSICAL CERTIFICATION} = \text{BLOCKED BY SAFETY GATE / NOT TESTED}$$
   No formatting occurs. Zero risk to Windows 11.

---

## 4. Dual-Execution Pipeline Strategy

To satisfy all prompt requirements while upholding absolute physical safety:
1. **Tier A — Pure UEFI QEMU Dual-Disk Engine (`tools/bofs/test_phase13_qemu.py`):**
   - Launches pure UEFI with two disks:
     - Disk 0: `build/atoms_uefi_test.img` (Bootloader & Kernel).
     - Disk 1: `build/bofs_dedicated_drive.img` (Dedicated 64 MB / 128 MB Raw Physical Block Device).
   - Simulates physical hardware block storage through the native block device driver.
   - Executes full automated lifecycle: Format $\rightarrow$ Mount $\rightarrow$ Create $\rightarrow$ Write $\rightarrow$ Readback $\rightarrow$ Reopen $\rightarrow$ Rename $\rightarrow$ Stat $\rightarrow$ Mkdir $\rightarrow$ Nested Files $\rightarrow$ Fragmentation $\rightarrow$ Unlink $\rightarrow$ Rmdir $\rightarrow$ Remount $\rightarrow$ Reboot Persistence $\rightarrow$ Crash Recovery $\rightarrow$ 1,000-Cycle Stress $\rightarrow$ Zero Resource Drift.
   - Captures COM1 serial telemetry and framebuffer screenshot.
2. **Tier B — Bare-Metal Hardware Target Engine (ASUS PRIME B750M-K):**
   - Boots over PXE (`tools/pxe_server.py`).
   - Scans physical NVMe controller, WD Blue SN5000 SSD, and partition table.
   - Displays live **BOFS PHYSICAL STORAGE SAFETY GATE** on screen.
   - Confirms all foreign partitions (Partitions 1–5) are write-locked with $0\text{ bytes written}$.
   - If dedicated test partition/USB is detected, permits operator confirmation; otherwise reports `PHYSICAL BOFS STORAGE = NOT AVAILABLE` without touching foreign disks.
   - Streams live UDP telemetry and captures native framebuffer screenshot.

---

## 5. Risk Analysis & Mitigation

| Risk | Severity | Mitigation |
|---|---|---|
| Accidental formatting of Windows 11 NTFS | Critical | Hardcoded check against GUIDs (`GUID_BASIC_DATA`, `GUID_EFI_SYSTEM`), partition name check, and `read_only = true` on parent disk. |
| Memory exhaustion during 1,000-cycle stress | High | Bounded allocation, strict slab/page recycling, and continuous resource drift monitoring. |
| Incomplete transaction on reboot | Medium | WAL recovery scanner automatically replays committed transactions and discards incomplete ones. |
| Silent corruption / CHKDSK auto-repair | Medium | Strictly forbidden by prompt Rule 59. System fails closed and halts on corruption detection. |

---

## 6. Suspected Fix & Implementation Scope (No Code)

The Phase 13 certified implementation will consist of:
1. `kernel/debug/bofs/bofs_phase13_certified_runner.h`: Data structures, safety gate definitions, test state machine, write/read ledgers.
2. `kernel/debug/bofs/bofs_phase13_certified_runner.c`: In-kernel physical storage probe, safety gate dialog, physical BOFS format engine, automated lifecycle runner, persistence verifier, and 4-panel ABDE UI integration.
3. `kernel/kernel.c`: `#define ATOMS_DEBUG_MODE_BOFS_PHASE13 17` and routing branch.
4. `build.ps1`: Clang rule for `bofs_phase13_certified_runner.c` and linker registry.
5. `tools/bofs/test_phase13_physical.py`: Automated host test matrix covering T01–T53 with 1,000-cycle stress.
6. `tools/bofs/test_phase13_qemu.py`: Pure UEFI QEMU runner with dedicated physical disk image.
7. `docs/BOFS/PHASE13_REAL_HARDWARE_CERTIFICATION.md` & `docs/BOFS/PHASE13_MASTER_CERTIFICATION_REPORT.md`.
