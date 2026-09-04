# ATOMS OS — ARCHITECTURE PATCH PLAN
## MISSION: REAL HARDWARE NVMe → NTFS → WINDOWS CROSS-BOOT WRITE VALIDATION
**Stage:** TASK 2 — ARCHITECT TEAM  
**Input:** `FORENSIC_REPORT.md` (Checkpoint: `9ba6e9c57181c2eac4e4bff38b6d88f2f037fc54`)  
**Safety Protocol:** Read-Only First. No modifications to certified subsystems. Strict phase isolation.

---

### 1. Scope of Architecture & Files to Modify

This patch introduces the native NVMe storage driver and GPT partition table parser to complete Stages 2 through 12 of the storage pipeline, proving the read-only path on real hardware before any write operations are allowed.

#### Files to Create:
1. `kernel/drivers/storage/nvme/nvme.h`
   - Defines NVMe 1.4 register offsets: `CAP`, `VS`, `CC`, `CSTS`, `AQA`, `ASQ`, `ACQ`, Doorbells.
   - Defines NVMe command formats: 64-byte SQE, 16-byte CQE.
   - Defines Identify Controller and Identify Namespace structures (`nvme_id_ctrl`, `nvme_id_ns`).
   - Defines telemetry export structures for controller and namespace metrics.

2. `kernel/drivers/storage/nvme/nvme.c`
   - Scans PCI bus for Class `0x01` (Mass Storage), SubClass `0x08` (Non-Volatile Memory), ProgIF `0x02` (NVM Express).
   - Enables PCI Memory Space and Bus Mastering.
   - Maps 64-bit BAR0 MMIO space (16KB / 4 pages).
   - Controller reset sequence (`CC.EN = 0`, wait for `CSTS.RDY = 0`).
   - Allocates 4KB page-aligned Admin SQ (64 entries) and Admin CQ (64 entries).
   - Configures `AQA`, `ASQ`, `ACQ`, sets `CC.EN = 1` with 64-byte SQE / 16-byte CQE / 4KB page size, waits for `CSTS.RDY = 1`.
   - Submits Admin Identify Controller (`CNS 0x01`): extracts Model Number, Serial, Firmware, Number of Namespaces.
   - Submits Admin Identify Namespace 1 (`CNS 0x00`): extracts `NSZE`, `NCAP`, `FLBAS`, computes sector size (`2^LBADS`) and total capacity.
   - Submits Admin `Create I/O CQ` (Opcode `0x05`) and `Create I/O SQ` (Opcode `0x01`) for Queue ID 1.
   - Implements `nvme_read_sectors()` using I/O Read (Opcode `0x02`).
   - Registers physical `BlockDevice` (`nvme0n1`).

3. `kernel/drivers/storage/partition/gpt.h`
   - Defines GPT Header (`EFI PART`, 92 bytes) and GPT Partition Entry (128 bytes).
   - Defines Microsoft Basic Data Partition GUID (`EBD0A0A2-B9E5-4433-87C0-68B6B72699C7`).
   - Defines partition device registration and boundary clamping structures.

4. `kernel/drivers/storage/partition/gpt.c`
   - Reads LBA 1 of the NVMe `BlockDevice` and verifies signature `0x5452415020494645ULL`.
   - Reads partition entries from LBA 2..33.
   - Locates the Microsoft Basic Data Partition (Windows 11 NTFS volume).
   - Instantiates and registers a partition sub-`BlockDevice` whose LBA 0 maps to `partition.start_lba` with strict boundary clamping to `partition.sector_count`.

#### Files to Modify:
5. `kernel/debug/storage_forensic_debug.c`
   - Calls `nvme_init()`.
   - Calls `gpt_scan_partitions()`.
   - Displays NVMe controller telemetry (Model, Serial, Firmware, Capacity, Sector Size, State).
   - Displays GPT partition discovery (Start LBA, Sector Count, Size in GB).
   - Calls `ntfs_mount()` on the partition block device.
   - Displays NTFS Volume validation (OEM ID, Cluster Size, MFT LCN).
   - Enumerates root directory entries to confirm read-only access to Windows 11 filesystem.
   - Emits visual telemetry screenshot via Port 9998.

6. `build.ps1`
   - Adds clang compilation rules for `kernel\drivers\storage\nvme\nvme.c` -> `build\nvme.o`.
   - Adds clang compilation rules for `kernel\drivers\storage\partition\gpt.c` -> `build\gpt.o`.
   - Adds `build/nvme.o` and `build/gpt.o` to `$lldRsp` kernel linker script.

---

### 2. Rationale & Design Decisions

- **Dynamic Discovery (Zero Hardcoding):**
  The driver dynamically scans the PCI bus for any Class `01:08:02` device, reads `CAP.DSTRD`, queries `NN` and `NSZE`, and determines the LBA format dynamically from `FLBAS`. Works uniformly across Haswell H81, ASUS B750M-K (WD SN570/SN580), QEMU NVMe, or any compliant controller.
- **Partition Clamping:**
  All I/O performed by the filesystem layer is dispatched to the partition sub-`BlockDevice`. The partition wrapper strictly enforces `lba + count <= sector_count`. This physically prevents any read or future write from touching the MBR, GPT header, or EFI partition.
- **Read-Only Gate:**
  In this phase, `nvme_write_sectors` is either guarded or stubbed. No sector modifications can take place until the read path is proven.

---

### 3. Expected Result

1. Kernel and bootloader compile with zero warnings and zero errors.
2. QEMU pre-flight validation succeeds with NVMe device enabled.
3. On the physical ASUS B750M-K hardware:
   - NVMe Controller detected at `PCI 02:00.0`.
   - WD NVMe SSD identified: Model and Serial dynamically read from hardware.
   - Namespace 1 registered: Capacity displayed accurately.
   - GPT Header verified at LBA 1.
   - Windows 11 Basic Data Partition discovered.
   - NTFS volume successfully mounted at `/windows` or partition node.
   - Windows 11 root directory files listed.
   - Visual ABDE diagnostic dashboard shows all green `PASS` indicators.

---

### 4. Risk Analysis & Rollback Plan

- **Risk Analysis:**
  - Operating system corruption risk: **ZERO**. No write commands are issued to the drive.
  - Regression risk to certified subsystems: **ZERO**. AHCI, BCM, ABDE, UEFI boot, USB HID, VMM, PMM, and SMP remain completely untouched.
- **Rollback Plan:**
  - If any unexpected behavior occurs, execute `git reset --hard 9ba6e9c57181c2eac4e4bff38b6d88f2f037fc54` to instantly return to the certified checkpoint.
