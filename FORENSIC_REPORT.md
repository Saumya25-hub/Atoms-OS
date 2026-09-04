# ATOMS OS — FORENSIC INVESTIGATION REPORT
## TASK 1: Physical Storage Discovery & AHCI Telemetry Audit

### 1. Executive Summary & Forensic Scope
- **Hardware Target**: ASUS B750M-K Motherboard (LGA1700), Intel Core i3-14100F.
- **Physical Validation Targets**:
  - SATA SSD (~128 GB)
  - SATA HDD (~512 GB)
  - NVMe M.2 Gen4 SSD (~512 GB)
- **Objective**: Phase 1 AHCI Physical Hardware Validation with truthful, comprehensive telemetry on real bare metal.
- **Rules Followed**: Phase Isolation (Rule 0). Zero hardcoding. Zero modifications to NTFS, FAT32, VFS, VMM, PMM, cursor, desktop, or certified subsystems.

---

### 2. Evidence Analysis (Bare-Metal ASUS B750M-K Boot)
1. **Physical Telemetry from Live Screen Capture (12:29 PM)**:
   - File: `artifacts/screenshots/screenshot_20260904_122900_s1.bmp`
   - Discovered PCI Storage Controllers: `RAID/Intel VMD | AHCI SATA | NVMe`.
   - SATA Disk 0 reported: `Capacity: 0 MB | Sector Size: 512 bytes | Total Sectors: 0`.
   - Disk 1 (SATA HDD ~512 GB) was completely invisible on screen.
   - Port-level diagnostics (PxSSTS, DET, IPM, link speed, PxSIG, model, serial, ABAR, B:D.F) were completely missing from the display.

2. **Root Cause 1 — ATA Command Header C Bit Premature Completion**:
   - In `kernel/drivers/storage/ahci/ahci.c` line 119, `hdr->c` was initialized to 1.
   - Per AHCI 1.3 Specification Section 4.2.2, setting `C=1` instructs the HBA to clear `PxCI` immediately upon receiving the link-layer `R_OK` primitive for the Command FIS, before device data transfer completes.
   - On physical hardware, this caused `ahci_identify_device()` to complete prematurely before the drive transmitted the 512-byte IDENTIFY data frame, resulting in all-zero buffers (0 MB, 0 sectors, blank model/serial).
   - This bug was addressed in commit `5a632e7` (`hdr->c = 0`), which validated successfully in QEMU, but has not yet been booted and verified on physical bare-metal hardware.

3. **Root Cause 2 — Single-Disk Hardcoded Rendering in Forensic Dashboard**:
   - In `kernel/debug/storage_forensic_debug.c`, lines 207-224 queried only `block_device_get(0)`.
   - When multiple SATA drives are present (e.g., SATA SSD + SATA HDD), `sata_disk1` is ignored by the renderer and never shown.
   - The dashboard lacked support for multi-device enumeration.

4. **Root Cause 3 — Missing Port-Level Telemetry Architecture in AHCI Driver**:
   - `ahci.h` and `ahci.c` lacked structured storage and export of controller PCI B:D.F, Vendor ID, Device ID, ABAR, and port-by-port states (PxSSTS, DET, IPM, Link Speed, PxSIG, IDENTIFY outcome, BlockDevice ID).
   - Non-connected or standby ports were discarded without recording status, preventing full port topology visibility.

---

### 3. Files Involved
1. `kernel/drivers/storage/ahci/ahci.h`
   - Lacks comprehensive telemetry structures for controller and implemented ports.
2. `kernel/drivers/storage/ahci/ahci.c`
   - Needs to record controller metadata and per-port forensic parameters during initialization.
   - Needs export accessors for forensic dashboard and telemetry inspection.
3. `kernel/debug/storage_forensic_debug.c`
   - Currently dedicated to VFS/NTFS tests rather than physical controller and multi-disk discovery.
   - Needs complete restructuring into a dedicated **PHYSICAL STORAGE CONTROLLER & DRIVE DISCOVERY DASHBOARD** that reports all ports, models, serials, capacities, and states.

---

### 4. Risk Analysis
- **System Stability**: Low risk. Telemetry query and formatting do not alter hardware command registers or memory paging.
- **Hardware Safety**: Zero risk. Read-only inquiry operations only (IDENTIFY DEVICE). No write commands, formatting, or sector changes.
- **Subsystem Isolation**: VFS, NTFS, FAT32, USB, VMM, and PMM remain untouched.

---

### 5. Suspected Fix Strategy (NO CODE)
1. **Extend AHCI Telemetry Model**:
   - Define structured types in `ahci.h` to hold Controller info (PCI B:D.F, Vendor/Device ID, ABAR, Cap, Version, Ports Implemented) and Port info (Port #, SSTS, DET, IPM, Link Speed, SIG, Identify Status, Model, Serial, Sector Count, Capacity MB, BlockDevice ID, Discovery State).
2. **Populate Port Data During AHCI Init**:
   - In `ahci.c`, record port status for every implemented port, whether a device is present, negotiating, or offline.
   - Populate model, serial, capacity, and BlockDevice ID upon successful IDENTIFY.
3. **Overhaul Storage Forensic Dashboard**:
   - Redesign `storage_forensic_debug.c` to render:
     - Section 1: PCI Storage Controllers (B:D.F, Vendor/Device, ABAR, Subsystem).
     - Section 2: AHCI Controller & Port Discovery Grid (Port #, PxSSTS, DET/IPM/SPD, PxSIG, Identify Status, Model, Serial, Capacity, BlockDevice ID, State).
     - Section 3: Registered Physical Block Devices (Iterate all registered drives dynamically: `sata_disk0`, `sata_disk1`, etc., with capacity and sector counts).
     - Section 4: Phase 1 Certification Verdict (Validating non-zero capacity, non-empty model/serial for all physical SATA devices).
4. **Pre-Flight in QEMU & Boot on Real Bare Metal via PXE**:
   - Compile cleanly with zero errors.
   - Pre-flight in QEMU UEFI mode.
   - Boot on ASUS B750M-K over PXE, capture visual telemetry BMP over UDP 9998, and verify dynamic discovery of SATA SSD (~128 GB) and SATA HDD (~512 GB).
