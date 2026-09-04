# ATOMS OS — FORENSIC INVESTIGATION REPORT
## MISSION: REAL HARDWARE NVMe → NTFS → WINDOWS CROSS-BOOT WRITE VALIDATION
**Stage:** PHASE 0 & PHASE 1 — FORENSIC AUDIT FIRST  
**Date:** 2026-09-04  
**Investigator:** FORENSIC TEAM (TASK 1)  
**Safety Protocol:** Read-Only Audit. NO CODE MODIFICATIONS. Zero Writes to Windows NVMe.

---

### 1. Executive Summary & Forensic Scope

- **Physical Target Hardware:**
  - **Motherboard:** ASUS B750M-K (Intel B760 Chipset, LGA1700)
  - **Processor:** Intel Core i3-14100F
  - **Physical NVMe:** Western Digital / SanDisk M.2 Gen4 NVMe SSD containing the user's live Windows 11 installation (`PCI 02:00.0`, VID: `0x15B7`, DID: `0x5017`, BAR0: `0x85000000`).
  - **Physical SATA Disks:** SATA SSD (`sata_disk0`, 465 GB) & SATA HDD (`sata_disk1`, 223 GB).
- **Core Forensic Objective:**
  - Audit the entire storage execution pipeline from physical PCI bus discovery down to NTFS volume mount, read, and write operations.
  - Trace each stage, verify hardware register contracts, pinpoint the **exact first broken link**, analyze risks to the real Windows 11 installation, and specify the requirements before any write test can be contemplated.

---

### 2. Phase 0 Safety & Git Checkpoint Record

- **Git Status Pre-Audit:** Checked and verified clean.
- **Certified Subsystems Protected/Frozen:**
  - UEFI Bootloader (`boot/uefi/`)
  - BCM, ABDE visual telemetry engine
  - USB HID / xHCI / PS/2 cursor engine
  - Syscall pointer validation gateway
  - VMM, PMM, Heap lifecycle
  - TSS, GDT, IDT, PIC, SMP APIC core
  - VFS unmount lifecycle
  - AHCI SATA controller and discovery (Phase 1 Certified)
- **Checkpoint Commit Hash:** `9ba6e9c57181c2eac4e4bff38b6d88f2f037fc54`
- **Checkpoint Commit Message:** `checkpoint: Phase 1 real hardware AHCI certified state before NVMe NTFS validation`

---

### 3. Complete Stage-by-Stage Forensic Audit: PCI → NVMe → BlockDevice → NTFS

| Stage # | Pipeline Stage | Source File | Function / Register / Contract | Expected Value | Observed Value | Status |
| :---: | :--- | :--- | :--- | :--- | :--- | :---: |
| **1** | **PCI Bus Discovery** | `kernel/core/pci/pci.c` | `pci_scan_bus()` / BaseClass `0x01`, SubClass `0x08`, ProgIF `0x02` | PCI Storage Controller enumeration | Discovered `PCI 02:00.0` (VID `0x15B7`, DID `0x5017`, Class `01:08:02`) | 🟢 **PASS** |
| **2** | **NVMe Controller Driver** | Missing (`kernel/drivers/storage/nvme/`) | `nvme_init()` / PCI Bus Mastering & MMIO enable | Dedicated driver managing NVMe lifecycle | **Driver does NOT exist** (`kernel/drivers/storage/` contains only `ahci/`) | 🔴 **FAIL (FIRST BROKEN LINK)** |
| **3** | **BAR0 MMIO Base Address** | `kernel/core/pci/pci.c` | `pci_read_bar(dev, 0)` / 64-bit Non-Prefetchable BAR | Valid physical MMIO address assigned by UEFI firmware | `0x85000000` (physically validated via live telemetry screenshot) | 🟢 **PASS (HW)** / 🔴 **FAIL (Unmapped)** |
| **4** | **NVMe Controller Registers (CAP/VS/CC/CSTS)** | Missing | Register Offsets `0x00` (CAP), `0x08` (VS), `0x14` (CC), `0x1C` (CSTS) | Read capabilities (MQES, TO, DSTRD), reset `CC.EN=0`, configure `AQA`/`ASQ`/`ACQ`, set `CC.EN=1` | Register access and state machine are not implemented | 🔴 **FAIL** |
| **5** | **Admin Queue (ASQ / ACQ)** | Missing | Offset `0x24` (AQA), `0x28` (ASQ), `0x30` (ACQ), Doorbell registers at `0x1000` | 4KB contiguous DMA ring buffers for Admin Submission & Completion Queues | Not allocated or configured | 🔴 **FAIL** |
| **6** | **Identify Controller** | Missing | Admin Opcode `0x06`, CNS `0x01` | 4096-byte `nvme_id_ctrl` containing Model Number, Serial, Firmware Revision, NN | Never issued | 🔴 **FAIL** |
| **7** | **Identify Namespace** | Missing | Admin Opcode `0x06`, CNS `0x00`, NSID `1` | 4096-byte `nvme_id_ns` containing `NSZE` (LBA count), `NCAP`, `FLBAS` | Never issued | 🔴 **FAIL** |
| **8** | **Namespace LBA Geometry & I/O Queues** | Missing | `Create I/O CQ` (Opcode 0x05), `Create I/O SQ` (Opcode 0x01), I/O Read (0x02) / Write (0x01) | Determine sector size (`2^LBADS`: 512B or 4096B), total capacity, and create QID 1 | Not implemented | 🔴 **FAIL** |
| **9** | **BlockDevice Interface** | `kernel/vfs/vfs_legacy/storage/include/block_device.h` | `block_device_register()` | Generic BlockDevice registering `nvme0n1` with `read`, `write`, `flush` callbacks | Only `sata_disk0` and `sata_disk1` registered | 🔴 **FAIL** |
| **10** | **GPT Partition Table Discovery** | Missing for non-USB storage (`usb_partition_manager.c` is USB-only MBR) | LBA 1 GPT Header (`EFI PART`), LBA 2-33 Partition Entries | Dynamic discovery of Basic Data Partition GUID (`EBD0A0A2-B9E5-4433-87C0-68B6B72699C7`) | Sub-block device or partition mapping not implemented | 🔴 **FAIL** |
| **11** | **NTFS Volume Detection** | `kernel/vfs/vfs_legacy/fs/ntfs/src/ntfs.c` | `ntfs_mount()`: reads LBA 0 of partition, checks OEM ID `'NTFS    '` & `0xAA55` | Validates BPB: bytes/sector, cluster size, MFT LCN | Logic exists, but currently hardcoded to read LBA 0 of raw device (fails on GPT disk) | 🟡 **SUSPECTED DEFECT** |
| **12** | **NTFS Read Path** | `kernel/vfs/vfs_legacy/fs/ntfs/src/ntfs.c` | `ntfs_mft_read_record()`, `ntfs_resolve_path()`, `ntfs_file_read()` | Reads MFT records, resolves `$ROOT` directory entries, reads file data | Fully implemented in `ntfs.c`, but blocked by missing NVMe block device and GPT partition offset | 🟢 **READY (Blocked)** |
| **13** | **NTFS Write Path** | `kernel/vfs/vfs_legacy/fs/ntfs/src/ntfs.c` | `ntfs_vfs_write()` & `ntfs_create_file()` | Write data to file and update MFT/bitmap | `ntfs_vfs_write()` is a stub (`return -1;`), but `ntfs_create_file()` implements MFT record allocation & data cluster write | 🟡 **PARTIALLY IMPLEMENTED** |

---

### 4. Detailed Forensic Breakdown

#### 4.1 Exact First Broken Link
- **First Broken Link:** **Stage 2 (NVMe Controller Driver)**.
- **Evidence:** `kernel/drivers/storage/` contains only `ahci/`. There is no `nvme/` directory, no `nvme.c`, and no `nvme.h`.
- **Impact:** Although the PCI discovery layer successfully discovers the physical NVMe SSD at `PCI 02:00.0` (VID: `0x15B7`, DID: `0x5017`, BAR0: `0x85000000`), no driver attaches to it, maps its MMIO registers, initializes its Admin queues, or creates a `BlockDevice`.

#### 4.2 The Second Broken Link: GPT Partition Table Parsing
- **Evidence:** The user's Windows 11 installation on NVMe is formatted with a **GUID Partition Table (GPT)**.
- **Critical Architectural Gap:** `ntfs_mount()` currently expects the NTFS boot sector at sector LBA 0 of the `BlockDevice` passed to it (`ntfs_read_sector(device, 0, 1, sector_buf)`).
- On a real Windows 11 NVMe disk:
  - LBA 0 = Protective MBR
  - LBA 1 = GPT Header (`EFI PART`)
  - LBA 2-33 = GPT Partition Entries (Partition 1: ESP FAT32, Partition 2: MSR, Partition 3: **Windows 11 NTFS**, Partition 4: Recovery).
  - The NTFS volume boot sector begins at the **Starting LBA** of Partition 3 (typically LBA `206848` or similar, depending on installation).
- **Forensic Finding:** Without a GPT partition parser that registers partition sub-devices (e.g., `nvme0n1p3`) or offsets LBA access to the partition's starting sector, passing `nvme0n1` directly to `ntfs_mount()` will fail with `Signature mismatch (Not an NTFS boot sector)` because LBA 0 contains the protective MBR.

#### 4.3 The Third Broken Link: NTFS Write vs. Create Pipeline
- **Evidence:** In `kernel/vfs/vfs_legacy/fs/ntfs/src/ntfs.c` line 1726:
  ```c
  static int ntfs_vfs_write(VFS_Node* node, uint64_t offset, uint32_t size, void* buffer) {
      (void)node; (void)offset; (void)size; (void)buffer;
      return -1; // Read-only filesystem in Phase 6
  }
  ```
- **Finding:** Modifying an existing file via standard `vfs_write()` is blocked as read-only. However, `ntfs_create_file()` (line 2112) is implemented and can allocate a new MFT record, allocate clusters in `$Bitmap`, write the file data payload, and insert an entry into the root directory index.
- **Safety Directive:** Creating exactly one isolated test file (`/ATOMS_WRITE_TEST.txt`) through a carefully controlled creation API is vastly safer for Windows 11 than in-place overwriting of existing metadata.

---

### 5. Risk Analysis & Windows 11 Safety Precautions

1. **Catastrophic Data Loss Risk (CRITICAL):**
   - The target NVMe contains the user's live, primary Windows 11 operating system.
   - Any raw sector write to the MBR, GPT header, partition table, EFI System Partition, or existing Windows system files/registry will corrupt the host OS and render the computer unbootable.
2. **Mitigation Measures Mandated:**
   - **MANDATORY READ-ONLY GATE:** Complete read-only verification (Stages 1 through 12: NVMe Controller -> Namespace -> BlockDevice -> GPT Parsing -> NTFS Mount -> Directory Read -> File Read) must be 100% operational and verified on physical hardware before ANY write operation is enabled.
   - **Partition Boundary Clamping:** BlockDevice partition wrappers must strictly clamp all LBA operations to the designated NTFS partition boundary (`start_lba <= LBA < start_lba + sector_count`). Any attempt to write outside the partition must trigger an immediate assertion/halt.
   - **Zero Writes to System Records:** The test must NOT touch MFT records 0 through 15 ($MFT, $MFTMirr, $LogFile, $Volume, $AttrDef, $Bitmap, $Boot, $BadClus, etc.), except for safe cluster allocation in $Bitmap.
   - **Deterministic Single File:** Exactly one file: `/ATOMS_WRITE_TEST.txt`.

---

### 6. Forensic Verdict & Recommendation

- **Verdict:** 🔴 **READ-ONLY PIPELINE INCOMPLETE (MISSING NVME DRIVER & GPT PARSER)**
- **Next Required Phase:** Task 2 (Architect Team) must produce `PATCH_PLAN.md` detailing:
  1. A standalone, high-reliability NVMe driver (`kernel/drivers/storage/nvme/`) implementing controller discovery, CAP/VS/CC/CSTS initialization, Admin SQ/CQ, Identify Controller, Identify Namespace, I/O SQ/CQ, and `BlockDevice` registration.
  2. A clean, generic GPT partition parser (`kernel/drivers/storage/partition/gpt.c`) that detects the Windows NTFS partition GUID and creates an offset-mapped partition BlockDevice.
  3. Physical read-only validation of Windows 11 NTFS volume enumeration on the real ASUS B750M-K hardware before moving to Phase 4 (Controlled Write).

**NO CODE MODIFICATIONS PERMITTED UNTIL PATCH_PLAN.md IS REVIEWED AND APPROVED.**
