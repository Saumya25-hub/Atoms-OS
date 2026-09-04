# ATOMS OS — PATCH REPORT
## TASK 3: Physical Storage Discovery & AHCI Telemetry Implementation

### 1. Scope & Plan Compliance
- **Input Plan**: `PATCH_PLAN.md`
- **Authorized Files**:
  1. `kernel/drivers/storage/ahci/ahci.h`
  2. `kernel/drivers/storage/ahci/ahci.c`
  3. `kernel/debug/storage_forensic_debug.c`
- **Unauthorized Modifications**: None. No other source files were touched. NTFS, FAT32, VFS, USB HID/xHCI, syscalls, VMM, PMM, and certified subsystems remain untouched.

---

### 2. File & Function Level Summary

#### File 1: `kernel/drivers/storage/ahci/ahci.h`
- **Modifications**:
  - Added enumeration `AHCIPortState` (`AHCI_PORT_STATE_NOT_IMPLEMENTED`, `AHCI_PORT_STATE_NO_DEVICE`, `AHCI_PORT_STATE_PHY_ONLINE`, `AHCI_PORT_STATE_DEVICE_INITIALIZED`, `AHCI_PORT_STATE_BDEV_REGISTERED`).
  - Added structure `AHCIPortTelemetry` recording: `port_num`, `implemented`, `ssts`, `det`, `ipm`, `spd`, `sig`, `identify_pass`, `model`, `serial`, `sector_count`, `sector_size`, `capacity_mb`, `bdev_id`, `state`.
  - Added structure `AHCIControllerTelemetry` recording: `controller_detected`, `pci_bus`, `pci_slot`, `pci_func`, `vendor_id`, `device_id`, `abar_phys`, `version`, `cap`, `ports_impl_mask`, `drive_count`, and array of 32 `AHCIPortTelemetry` structures.
  - Declared getters `ahci_get_controller_telemetry(void)` and `ahci_get_port_telemetry(uint8_t port_num)`.

#### File 2: `kernel/drivers/storage/ahci/ahci.c`
- **Functions Modified**:
  - `ahci_init_port()`:
    - Records per-port telemetry in `s_telemetry.ports[port_num]`.
    - Populates `ssts`, `det`, `spd`, `ipm`, `sig`.
    - Sets state: `AHCI_PORT_STATE_NO_DEVICE` if DET != 3; `AHCI_PORT_STATE_PHY_ONLINE` if link is established.
    - Upon ATA IDENTIFY completion: sets `identify_pass = true`, records ASCII model (trimmed), serial (trimmed), sector count, sector size, and capacity in MB.
    - Upon `block_device_register()`: records `bdev_id` and sets `AHCI_PORT_STATE_BDEV_REGISTERED`.
  - `ahci_init()`:
    - Clears and initializes `s_telemetry`.
    - Captures PCI location (`bus`, `slot`, `func`), `vendor_id`, `device_id`, and physical ABAR.
    - Captures `cap`, `version`, and `ports_impl_mask`.
    - Iterates implemented ports, updating `s_telemetry.drive_count`.
  - Added getters:
    - `ahci_get_controller_telemetry()`: returns pointer to `s_telemetry`.
    - `ahci_get_port_telemetry()`: returns pointer to specified port telemetry entry.

#### File 3: `kernel/debug/storage_forensic_debug.c`
- **Functions Modified**:
  - `render_dashboard_shell()`: Updated dashboard layout for physical storage discovery, scaling to screen width (980px panel card).
  - `storage_dbg_render_hex32()`: Added 32-bit hex renderer for MMIO ABAR, PxSIG, and Ports Implemented mask.
  - `storage_forensic_debug_run()`:
    - **Step 1**: Enumerate PCI storage controllers dynamically (AHCI SATA, NVMe, Intel VMD) with B:D.F, Vendor ID, Device ID, BAR0, and state `CONTROLLER DETECTED`.
    - **Step 2**: Initialize AHCI and render comprehensive port telemetry (Port #, SSTS, DET, IPM, Link Speed, SIG, IDENTIFY status, Model, Serial, Capacity GB/MB, Sector Count, BlockDevice ID, and State `BLOCKDEVICE REGISTERED`).
    - **Step 3**: Enumerate all registered BlockDevices in `block_device_count()` dynamically (`sata_disk0`, `sata_disk1`, etc.), displaying capacities and read-only status.
    - **Step 4**: Compute Phase 1 Physical Certification Verdict:
      - Validates non-zero capacity, non-empty model/serial, and valid sector count.
      - Displays PASS verdict and dynamic parsing confirmation.
    - Emits visual telemetry frame request (`atoms_screenshot_request(1)`) and runs continuous diagnostic spinner heartbeat loop.
