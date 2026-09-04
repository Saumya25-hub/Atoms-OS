# ATOMS OS — ARCHITECTURE PATCH PLAN
## TASK 2: Physical Storage Discovery & AHCI Telemetry Reporting

### 1. Objective & Scope
- **Target**: ASUS B750M-K (Intel Core i3-14100F).
- **Physical Validation Expected**: SATA SSD (~128 GB) and SATA HDD (~512 GB).
- **Phase**: PHASE 1 ONLY — AHCI Physical Validation. (NVMe and VMD postponed until Phase 2).
- **Constraints**: ZERO hardcoding. ZERO modifications to NTFS, FAT32, VFS, USB HID/xHCI, syscalls, VMM, PMM, cursor, compositor, or desktop.

---

### 2. Files to Modify (Exclusively)
1. `kernel/drivers/storage/ahci/ahci.h`
2. `kernel/drivers/storage/ahci/ahci.c`
3. `kernel/debug/storage_forensic_debug.c`

---

### 3. Detailed Architectural Modifications (NO CODE)

#### A. Telemetry Architecture in `ahci.h`
- Define `AHCIPortState` enumeration:
  - `AHCI_PORT_STATE_UNIMPLEMENTED`
  - `AHCI_PORT_STATE_NO_DEVICE`
  - `AHCI_PORT_STATE_LINK_UP`
  - `AHCI_PORT_STATE_IDENTIFIED`
  - `AHCI_PORT_STATE_BDEV_REGISTERED`
- Define `AHCIPortTelemetry` structure:
  - `port_num` (uint8_t)
  - `implemented` (bool)
  - `ssts` (uint32_t)
  - `det` (uint8_t)
  - `ipm` (uint8_t)
  - `spd` (uint8_t)
  - `sig` (uint32_t)
  - `identify_ok` (bool)
  - `model` (char[41])
  - `serial` (char[21])
  - `sector_count` (uint64_t)
  - `sector_size` (uint32_t)
  - `capacity_mb` (uint64_t)
  - `bdev_id` (int)
  - `state` (AHCIPortState)
- Define `AHCIControllerTelemetry` structure:
  - `detected` (bool)
  - `bus`, `slot`, `func` (uint8_t)
  - `vendor_id`, `device_id` (uint16_t)
  - `abar_phys` (uint64_t)
  - `version` (uint32_t)
  - `cap` (uint32_t)
  - `ports_impl_mask` (uint32_t)
  - `port_count` (uint8_t)
  - `drive_count` (uint8_t)
  - `ports[MAX_AHCI_PORTS]` (AHCIPortTelemetry)
- Declare getter functions:
  - `const AHCIControllerTelemetry* ahci_get_controller_telemetry(void);`
  - `const AHCIPortTelemetry* ahci_get_port_telemetry(uint8_t port_num);`

#### B. Telemetry Population in `ahci.c`
- Initialize and populate `AHCIControllerTelemetry` during `ahci_init()`:
  - Record PCI B:D.F, Vendor ID, Device ID, ABAR, Version, Capabilities, and Ports Implemented mask.
- In `ahci_init_port()`:
  - Record telemetry for every implemented port, regardless of whether a drive is attached.
  - Read `PxSSTS`, parse DET (bits 3:0), SPD (bits 7:4), and IPM (bits 11:8).
  - If DET indicates physical presence (DET == 3):
    - Read `PxSIG`.
    - If `PxSIG == SATA_SIG_ATA`:
      - Execute `ahci_identify_device()`.
      - If successful, record model (40 chars trimmed), serial (20 chars trimmed), sector count (LBA48/LBA28), sector size, and capacity in MB.
      - Register `BlockDevice` and record assigned Global Block ID.
      - Update state to `AHCI_PORT_STATE_BDEV_REGISTERED`.
    - If IDENTIFY fails, record state as `AHCI_PORT_STATE_LINK_UP`.
  - If DET indicates no device, record state as `AHCI_PORT_STATE_NO_DEVICE`.

#### C. Dedicated Storage Discovery Dashboard in `storage_forensic_debug.c`
- Replace previous VFS/NTFS benchmark layout with the required **Physical Storage Discovery Dashboard**:
  - **Header**: System identification (ASUS B750M-K / Intel Core i3-14100F).
  - **Panel 1 — Storage Controllers Discovered (PCI Topology)**:
    - Lists discovered controllers: AHCI SATA, NVMe, and RAID/VMD with Bus:Device.Function, Vendor/Device ID, and MMIO Base Addresses.
  - **Panel 2 — AHCI Controller & Implemented Port Status**:
    - Displays Controller ABAR, Version, and Port Bitmask.
    - Loops through implemented ports, rendering:
      - Port #
      - PxSSTS, DET, IPM, Link Speed (e.g. "Gen 3 (6.0 Gbps)" / "Gen 2 (3.0 Gbps)")
      - PxSIG
      - IDENTIFY status (PASS/FAIL/NONE)
      - State (e.g. `BLOCKDEVICE REGISTERED`, `DEVICE DETECTED`, `NO DEVICE`)
  - **Panel 3 — Physical Drive Discovery & BlockDevice Registry**:
    - Enumerates all registered SATA block devices dynamically:
      - Device Name (e.g. `sata_disk0`, `sata_disk1`)
      - Drive Model (real ASCII string from IDENTIFY)
      - Serial Number (real ASCII string from IDENTIFY)
      - Capacity (formatted in GB / MB)
      - Total Sector Count & Sector Size (512 bytes)
      - State: `BLOCKDEVICE REGISTERED`
  - **Panel 4 — Phase 1 Certification Verdict**:
    - Validates:
      1. AHCI Controller Detected.
      2. At least one SATA device identified with non-zero capacity, non-empty model, and valid sector count.
      3. Distinguishes whether multiple drives (SSD + HDD) are discovered.
    - If pass: Displays `PHASE 1 AHCI VERDICT: PASS (SATA DRIVES DISCOVERED & REGISTERED)`.
  - Maintains the active diagnostic spinner and UDP screenshot transmission (`atoms_screenshot_request(1)`).

---

### 4. Risk Assessment & Controls
- **Memory Footprint**: `AHCIControllerTelemetry` consumes ~4 KB in kernel BSS (negligible).
- **Execution Overhead**: Port probing runs once at boot. Zero impact on runtime.
- **Safety**: Purely passive telemetry. Zero sector writes, zero partition modifications.

---

### 5. Rollback Plan
- Git checkout `kernel/drivers/storage/ahci/ahci.h`, `kernel/drivers/storage/ahci/ahci.c`, and `kernel/debug/storage_forensic_debug.c`.
