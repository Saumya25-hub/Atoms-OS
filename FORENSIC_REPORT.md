# FORENSIC REPORT — ATOMS OS PHASE 1A REAL STORAGE BRING-UP FAILURE

**Case ID**: `CASE_20260904_REAL_STORAGE_FORENSIC`  
**Date**: September 4, 2026  
**Investigator**: Kernel Forensic Team  
**Audit Target**: Physical Storage Pipeline (PCI ➔ Controller Init ➔ Block Device ➔ Disk Manager ➔ Partition Parser ➔ Filesystem Detection ➔ VFS Mount)  
**Target Hardware**: ASUS B750M-K Motherboard (Pure UEFI Mode) | Intel Core i3-14100F (LGA1700 Architecture)  
**Verdict**: 🔴 **REAL STORAGE DISCOVERY NOT PROVEN / FAILED** (Pre-Fix Forensic State)

---

## 1. Executive Summary & Incident Autopsy

On September 4, 2026, the physical bare-metal test of ATOMS OS on the ASUS B750M-K (Intel Core i3-14100F LGA1700) booted cleanly in native UEFI GOP mode (1920×1080 32bpp) and streamed live framebuffer telemetry back to the PXE diagnostic server (`screenshot_20260904_013403_s1.png`). 

While the diagnostic screen displayed `STORAGE BRING-UP VERDICT: PASS`, a rigorous forensic investigation of the runtime call path and kernel source code demonstrates that **this was a false PASS**. The operating system successfully initialized its diagnostic UI and mounted a 1 MB in-memory RAM fallback (`dummyfs`) at root (`/`), but **zero physical storage controllers were initialized, zero physical disks were discovered, and no real block device ever reached the partition parser, NTFS driver, or VFS layer**.

The real storage result is authoritatively classified as:
### 🔴 REAL STORAGE DISCOVERY NOT PROVEN / FAILED

---

## 2. Forensic Objective & Chain Trace

The objective of this forensic investigation is to identify the **exact first broken link** in the end-to-end storage pipeline:

```text
[1. PCI Bus Enumeration]                     🟢 CONFIRMED WORKING
       │
       ▼
[2. Storage Controller Detection]            🟢 CONFIRMED WORKING
       │
       ▼
[3. Storage Controller Initialization]       🔴 CONFIRMED FAILURE (EXACT FIRST BROKEN LINK)
       │
       ▼
[4. Physical Device Discovery]               🔴 CONFIRMED FAILURE (0 drives discovered)
       │
       ▼
[5. Block Device Registration]               🔴 CONFIRMED FAILURE (0 block devices registered)
       │
       ▼
[6. Disk Manager Partition Scan]             🔴 CONFIRMED FAILURE (Never invoked)
       │
       ▼
[7. Partition Discovery (MBR/GPT)]           🔴 CONFIRMED FAILURE (Sector 0 never read)
       │
       ▼
[8. Filesystem Detection]                    🔴 CONFIRMED FAILURE (Never called)
       │
       ▼
[9. NTFS Driver Activation]                  🔴 CONFIRMED FAILURE (Never reached)
       │
       ▼
[10. VFS Real Mount]                         🟡 PARTIAL / FALLBACK ONLY (DummyFS in RAM)
```

---

## 3. Step-by-Step Forensic Analysis

### STEP 1 — PCI FORENSICS: Controller Detection & "Unknown" Analysis

#### Runtime Evidence
The real hardware diagnostic screen displayed:
```text
PCI: Unknown | AHCI SATA | NVMe
```

#### Code Trace
In [`kernel/debug/storage_forensic_debug.c:144-180`](file:///d:/Signatures_OS/kernel/debug/storage_forensic_debug.c#L144-L180):
```c
for (uint32_t i = 0; i < pci_count; i++) {
    PCIDevice* dev = pci_get_device(i);
    if (!dev) continue;

    if (dev->base_class == 0x01) { // Mass Storage
        storage_ctrl_count++;
        const char* type_str = "Unknown";
        if (dev->sub_class == 0x01) type_str = "IDE";
        else if (dev->sub_class == 0x06) type_str = "AHCI SATA";
        else if (dev->sub_class == 0x08) type_str = "NVMe";
```

#### Root Cause of "Unknown"
The PCI Local Bus Specification defines the following subclasses for Base Class `0x01` (Mass Storage Controller):
- `0x00`: SCSI Bus Controller
- `0x01`: IDE Controller
- `0x02`: Floppy Disk Controller
- `0x03`: IPI Bus Controller
- `0x04`: RAID Controller / Intel RST VMD (Volume Management Device)
- `0x05`: ATA Controller
- `0x06`: Serial ATA Controller (AHCI)
- `0x07`: Serial Attached SCSI (SAS)
- `0x08`: Non-Volatile Memory Subsystem (NVMe)
- `0x80`: Other Mass Storage Controller

On the physical ASUS B750M-K (Intel 700-series chipset), the PCI scan encountered **three separate storage devices** with `base_class == 0x01`:
1. **Device #1**: `base_class == 0x01`, `sub_class` was neither `0x01`, `0x06`, nor `0x08`. On modern LGA1700 motherboards, Intel VMD / RST RAID Controller is exposed at Class `0x01`, Subclass `0x04` (RAID) or `0x80` (Other). Because the `if/else if` chain lacked a case for `0x04` or other subclasses, `type_str` remained defaulted to `"Unknown"`. This populated the first element: `"PCI: Unknown"`.
2. **Device #2**: `base_class == 0x01`, `sub_class == 0x06` (Intel SATA AHCI Controller, Prog-IF `0x01`). This appended `" | AHCI SATA"`.
3. **Device #3**: `base_class == 0x01`, `sub_class == 0x08` (PCIe M.2 NVMe Controller, Prog-IF `0x02`). This appended `" | NVMe"`.

**Conclusion**: PCI enumeration is working correctly and accurately identifies the physical hardware present on the bus. The "Unknown" label is simply a missing case in the diagnostic string formatter for PCI Subclass `0x04` (RAID / Intel VMD).

---

### STEP 2 — AHCI / SATA PATH: The Legacy ATA Mismatch

#### Code Trace & Evidence
In [`kernel/vfs/vfs_legacy/storage/src/disk_manager.c:97-113`](file:///d:/Signatures_OS/kernel/vfs/vfs_legacy/storage/src/disk_manager.c#L97-L113):
```c
void disk_manager_init(void) {
    display_print("\n[DSK] Initializing Disk Manager...\n");
    block_device_init();
    ata_init(); // <-- ONLY legacy ATA is called!
    int initial_devices = block_device_count();
    for (int i = 0; i < initial_devices; i++) {
        mbr_parse(i);
    }
}
```
And in [`kernel/drivers/storage_legacy/storage/src/ata.c:209-217`](file:///d:/Signatures_OS/kernel/drivers/storage_legacy/storage/src/ata.c#L209-L217):
```c
void ata_init(void) {
    display_print("[ATA] Probing Physical ATA Storage Controller Drives...\n");
    ata_drive_count = 0;

    ata_probe_single_drive(ATA_PRIMARY_IO_BASE, ATA_PRIMARY_CTRL_BASE, true, "ATA_PM");
    ata_probe_single_drive(ATA_PRIMARY_IO_BASE, ATA_PRIMARY_CTRL_BASE, false, "ATA_PS");
    ata_probe_single_drive(ATA_SECONDARY_IO_BASE, ATA_SECONDARY_CTRL_BASE, true, "ATA_SM");
    ata_probe_single_drive(ATA_SECONDARY_IO_BASE, ATA_SECONDARY_CTRL_BASE, false, "ATA_SS");
}
```
Where:
- `ATA_PRIMARY_IO_BASE` = `0x1F0`
- `ATA_PRIMARY_CTRL_BASE` = `0x3F6`
- `ATA_SECONDARY_IO_BASE` = `0x170`
- `ATA_SECONDARY_CTRL_BASE` = `0x376`

#### Forensic Questions & Answers
1. **Does ATOMS actually initialize AHCI?**  
   **NO.** There is no AHCI driver in ATOMS OS. A complete repository search reveals zero implementation files for AHCI.
2. **Does it map the controller BAR?**  
   **NO.** PCI BAR5 (ABAR) is read into `dev->bars[5]` in `pci.c`, but it is never mapped into the VMM page tables via MMIO and never dereferenced.
3. **Does it detect ports?**  
   **NO.** The AHCI Ports Implemented (`PI`) register at ABAR offset `0x0C` is never read.
4. **Does it detect link state?**  
   **NO.** Port SATA Status (`PxSSTS`) is never inspected.
5. **Does it issue IDENTIFY?**  
   - In AHCI: **NO.** (No FIS constructed).
   - In Legacy ATA: It attempts to issue `ATA_CMD_IDENTIFY` (`0xEC`) via legacy ISA I/O port `0x1F7`.
6. **Does IDENTIFY return successfully?**  
   **NO.** On the LGA1700 ASUS B750M-K, legacy ISA I/O ports `0x1F0` and `0x170` do not exist in hardware. Reading `0x1F7` returns floating bus (`0xFF`). The loop in `ata_probe_single_drive()` aborts immediately at line 164 (`if (status == 0 || status == 0xFF) return false;`).
7. **Does it create a BlockDevice?**  
   **NO.** `ata_drive_count` remains 0. No `BlockDevice` is populated.
8. **Does that BlockDevice get registered?**  
   **NO.** `block_device_register()` is never called for any physical drive.
9. **Does disk_manager receive it?**  
   **NO.** `block_device_count()` returns 0.

**Physical Hardware Proof**: Modern Intel B760 motherboards operate exclusively in native AHCI/PCIe mode under UEFI. The legacy IDE/ATA PIO driver is 100% physically incompatible with this motherboard.

---

### STEP 3 — NVMe PATH: Layer-by-Layer Verification

Trace of the NVMe storage pipeline:

| Layer | State | Evidence & Reality |
| :--- | :--- | :--- |
| **1. PCI Detection** | 🟢 **CONFIRMED** | Detected on bus with Class `0x01`, Subclass `0x08`, Prog-IF `0x02`. Appears in `ctrl_summary`. |
| **2. BAR Discovery** | 🟡 **PARTIAL** | BAR0 (64-bit MMIO) base address read by `pci_parse_bars()`. However: (a) `PCI_COMMAND_MEMORY` and `PCI_COMMAND_MASTER` were **never enabled** for storage devices in `pci.c:177`, and (b) BAR0 was **never mapped** into kernel virtual memory. |
| **3. Controller Init** | 🔴 **MISSING** | No NVMe registers (CAP, VS, CC, CSTS) are ever accessed. Controller reset and `CC.EN` enablement are absent. |
| **4. Admin Queue** | 🔴 **MISSING** | No Submission Queue (ASQ) or Completion Queue (ACQ) circular DMA buffers exist. |
| **5. Identify Controller** | 🔴 **MISSING** | Admin Identify Controller command (CNS `0x01`) is never issued. |
| **6. Namespace Discovery**| 🔴 **MISSING** | Identify Active Namespaces (CNS `0x02`) or Namespace 1 (CNS `0x00`) is never issued. LBA format and capacity are unknown. |
| **7. Block Device Reg** | 🔴 **MISSING** | Zero NVMe `BlockDevice` instances are created or registered. |
| **8. Disk Manager** | 🔴 **NOT REACHED** | `disk_manager` has zero NVMe bindings. |

---

### STEP 4 — DISK MANAGER FORENSICS: Why "No MBR partitions mapped" Appears

In [`kernel/debug/storage_forensic_debug.c:229-280`](file:///d:/Signatures_OS/kernel/debug/storage_forensic_debug.c#L229-L280):
```c
int log_part_count = disk_manager_get_logical_drive_count();
if (log_part_count > 0) {
    ...
} else {
    abde_render_string(40, part_row_y, "No MBR Partitions mapped on primary physical disk.", COLOR_LABEL, COLOR_PANEL);
}
```

#### Authoritative Proof of Root Cause
The message appears because `log_part_count == 0`.
`log_part_count` is 0 because `disk_manager_register_partition()` was never called.
`disk_manager_register_partition()` was never called because `mbr_parse()` was never called.
`mbr_parse()` was never called because `initial_devices == 0` in `disk_manager_init()`:
```c
int initial_devices = block_device_count(); // Returns 0!
for (int i = 0; i < initial_devices; i++) {
    mbr_parse(i); // NEVER RUNS!
}
```

**Evaluation of Options**:
- **A. No physical block device exists?** -> **YES (Consequence)**. `block_device_count() == 0`.
- **B. Disk_manager is not receiving the device?** -> **YES (Consequence)**. Registry is empty.
- **C. Only legacy ATA is searched?** -> **YES (Direct Code Bug)**. `disk_manager_init()` explicitly hardcodes only `ata_init()`.
- **D. AHCI is not initialized?** -> **YES (Root Cause)**. Real SATA controller on motherboard is AHCI, but no AHCI driver exists.
- **E. NVMe is not initialized?** -> **YES (Root Cause)**. Real M.2 SSD on motherboard is NVMe, but no NVMe driver exists.
- **F. Partition parsing is wrong?** -> **NO**. Partition parsing (`mbr_parse()`) was never executed.
- **G. Initialization order is wrong?** -> **Secondary**. Even if called in different order, missing drivers yield 0 devices.

---

### STEP 5 — PARTITION TABLE: MBR vs. GPT

The GPT parser implemented in `kernel/vfs/vfs_legacy/storage/src/mbr.c:71-115` correctly handles Protective MBR (Type `0xEE`), validates `"EFI PART"` signature at LBA 1, and maps partitions. This was verified in QEMU.

However, on physical hardware:
- `block_device_count()` was 0.
- `mbr_parse()` was never invoked.
- Sector 0 was never read.
- **The real disk never reached the partition parser.**
- **Finding**: Partition parsing is NOT broken; it was simply never supplied with a physical block device. As mandated, **GPT parsing must NOT be modified**.

---

### STEP 6 — NTFS DRIVER

- The NTFS driver (`kernel/vfs/vfs_legacy/fs/ntfs/src/ntfs.c`) mounts volumes via `ntfs_mount(BlockDevice *dev)`.
- Because `log_part_count` was 0, `vfs_detect_fs()` was never called on a physical partition.
- `ntfs_detect()` and `ntfs_mount()` were never invoked.
- **Finding**: NTFS is NOT the root cause. It is dormant and waiting for a valid block device. As mandated, **NTFS must NOT be modified**.

---

### STEP 7 — DUMMYFS FALLBACK & THE FALSE "PASS" VERDICT

#### Code Trace of False Verdict
In [`kernel/debug/storage_forensic_debug.c:297-324, 413-421`](file:///d:/Signatures_OS/kernel/debug/storage_forensic_debug.c#L297-L324):
```c
if (mount_status != 0) {
    // Safe Temporary Fallback: mount DummyFS at / to ensure root is always valid
    is_temporary_fallback = true;
    mount_path_target = "/";
    detected_fs = "dummyfs";
    int dummy_dev_id = block_device_count();
    static BlockDevice s_fallback_bdev = {
        .name = "fallback_ramdisk", .sector_size = 512, .sector_count = 2048, .read_only = true
    };
    int f_id = block_device_register(&s_fallback_bdev);
    mount_status = vfs_mount_fs("/", f_id, "dummyfs");
}

...

bool overall_pass = (mount_status == 0) && read_pass && close_pass;
if (overall_pass) {
    com1_puts("[STORAGE_BRINGUP] FINAL VERDICT: PASS (Storage Verified)\r\n");
    abde_render_string(40, 650, "STORAGE BRING-UP VERDICT: PASS", COLOR_PASS, COLOR_PANEL);
}
```

#### Forensic Conclusion
When physical storage failed, the kernel safely fell back to mounting `dummyfs` on an ephemeral RAM block device. Because `dummyfs` successfully mounted at `/` (`mount_status == 0`), the condition `(mount_status == 0) && read_pass && close_pass` evaluated to **TRUE**, erroneously awarding a full physical storage PASS.

A fallback RAM filesystem proves only that the in-memory VFS abstraction is alive; **it proves nothing about physical storage bring-up**.

The diagnostic classification must be updated in the implementation plan so that when `is_temporary_fallback == true`, the dashboard renders:
```text
REAL STORAGE: NOT DETECTED
FALLBACK ROOT: ACTIVE
VERDICT: 🔴 REAL STORAGE DISCOVERY NOT PROVEN / FAILED
```

---

## 4. Subsystem Status Matrix

| Subsystem | Status | Current Reality |
| :--- | :---: | :--- |
| **1. PCI Bus Enumeration** | 🟢 **PASS** | Successfully enumerates all PCI devices; discovers Unknown (RAID/VMD), AHCI SATA, and NVMe controllers. |
| **2. Storage Controller Detection** | 🟢 **PASS** | PCI Base Class `0x01` correctly identified for all storage devices. |
| **3. Controller Initialization** | 🔴 **FAIL** | **EXACT FIRST BROKEN LINK**. No AHCI driver. No NVMe driver. `pci.c` does not enable MMIO/BusMaster for Class `0x01`. |
| **4. Physical Device Discovery** | 🔴 **FAIL** | `ata_init()` probes non-existent legacy ports `0x1F0`/`0x170`. 0 physical drives discovered. |
| **5. Block Device Registration** | 🔴 **FAIL** | Zero physical `BlockDevice` instances registered. `block_device_count() == 0`. |
| **6. Disk Manager** | 🔴 **FAIL** | Only calls `ata_init()`. Receives 0 devices. `logical_drive_count == 0`. |
| **7. Partition Parser** | 🔴 **FAIL** | Never called (`mbr_parse` loop has 0 iterations). |
| **8. Filesystem Detection** | 🔴 **FAIL** | Never called (`vfs_detect_fs` requires active logical drive). |
| **9. NTFS Driver** | 🔴 **FAIL** | Untouched, never reached. |
| **10. VFS Mount** | 🟡 **PARTIAL** | Ephemeral RAM `dummyfs` mounted at `/`. Real physical storage mount never attempted. |

---

## 5. The Exact First Broken Link

> [!CAUTION]
> ### ROOT CAUSE IDENTIFICATION
> The exact first broken link in the real hardware storage chain is:  
> **LINK #3: STORAGE CONTROLLER INITIALIZATION**
> 
> Specifically:
> 1. The ASUS B750M-K LGA1700 motherboard has **no legacy IDE controller**; its SATA and M.2 interfaces operate in native **AHCI** and **NVMe** modes.
> 2. `disk_manager_init()` only calls `ata_init()`, which exclusively performs ISA port I/O to ports `0x1F0` / `0x170`.
> 3. ATOMS OS contains **no AHCI controller driver** and **no NVMe controller driver**.
> 4. `kernel/core/pci/pci.c` fails to call `pci_enable_memory_space()` and `pci_enable_bus_mastering()` for Class `0x01` (Storage) controllers.
> 
> All subsequent failures (0 physical drives, 0 block devices, 0 partitions, unmounted NTFS) are direct cascading symptoms of this single broken link.

---

## 6. Files Involved in the Failure Chain

1. [`kernel/debug/storage_forensic_debug.c`](file:///d:/Signatures_OS/kernel/debug/storage_forensic_debug.c):
   - Missing PCI Subclass `0x04` (RAID / Intel VMD) identification string.
   - Conflates DummyFS fallback mount with real storage PASS.
2. [`kernel/core/pci/pci.c`](file:///d:/Signatures_OS/kernel/core/pci/pci.c):
   - Lines 177–182: Only enables Bus Master and MMIO for USB (`0x0C`) and Network (`0x02`), omitting Storage (`0x01`).
3. [`kernel/vfs/vfs_legacy/storage/src/disk_manager.c`](file:///d:/Signatures_OS/kernel/vfs/vfs_legacy/storage/src/disk_manager.c):
   - `disk_manager_init()` hardcodes `ata_init()`, ignoring AHCI and NVMe controllers present in the system.
4. [`kernel/drivers/storage_legacy/storage/src/ata.c`](file:///d:/Signatures_OS/kernel/drivers/storage_legacy/storage/src/ata.c):
   - Legacy PIO driver incompatible with modern LGA1700 / B760 chipset hardware.

---

## 7. Suspected Architectural Fix Strategy (NO CODE IN THIS PHASE)

Per RULE 0, **zero code modifications or patches have been made**. The suspected path forward for the Architecture Team (`PATCH_PLAN.md`) is:

1. **Dashboard Classification Update**:
   - Disentangle DummyFS fallback from physical storage verdict.
   - When running on fallback root, render:
     `REAL STORAGE: NOT DETECTED`
     `FALLBACK ROOT: ACTIVE`
     `VERDICT: 🔴 REAL STORAGE DISCOVERY NOT PROVEN / FAILED`
2. **PCI Subclass String Expansion**:
   - Add Subclass `0x04` ("RAID / Intel VMD") and `0x00` ("SCSI") to `storage_forensic_debug.c` so the hardware accurately reports the controller identity instead of "Unknown".
   - Enable Bus Master (`PCI_COMMAND_MASTER`) and Memory Space (`PCI_COMMAND_MEMORY`) on PCI Class `0x01` devices.
3. **Native AHCI SATA Driver (Phase 1B Bring-Up)**:
   - Implement native AHCI 1.3 driver mapping ABAR (BAR5) via VMM.
   - Configure Global HBA registers (`GHC.HR`, `GHC.AE`).
   - Iterate active ports from Ports Implemented (`PI`).
   - Detect SATA link status via `PxSSTS.DET == 3`.
   - Issue IDENTIFY DEVICE FIS (`0x27`, Command `0xEC`).
   - Parse dynamic capacity, sector size, and serial number.
   - Register native `BlockDevice` (`ahci_read_sectors`, `ahci_write_sectors`).
4. **Disk Manager Multi-Driver Integration**:
   - Update `disk_manager_init()` to invoke both `ahci_init()` and `nvme_init()` in addition to legacy `ata_init()`.
   - Allow any registered block device to be scanned by `mbr_parse()`.

---

**Certified by Kernel Forensic Team**  
*ATOMS OS Engineering Protocol V1 — RULE 0 Phase Isolation Active*  
*Status: INVESTIGATION COMPLETE — AWAITING ARCHITECTURAL APPROVAL*
