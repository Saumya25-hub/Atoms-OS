# ATOMS OS — MASTER STORAGE ARCHITECTURE

**Status**: Authoritative Master Architecture  
**Owner**: Kernel Storage & I/O Architecture Team  
**Classification**: Operating System Core Specification  
**Version**: 1.0 (Phase 1A Baseline)  
**Target Hardware Baseline**: ASUS B750M-K (Intel Core i3-14100F LGA1700), QEMU x86_64 UEFI

---

## 1. Architectural Philosophy & Independence

ATOMS OS is an independent operating system with its own architectural principles, memory management, driver interfaces, and execution semantics. 

Mature operating systems—specifically **Linux** and **Windows NT**—serve as **engineering references only**. We study how they achieve clean hardware abstraction, asynchronous request orchestration, bus enumeration, and filesystem separation, but ATOMS OS implements its own lightweight, modular, and verifiable equivalents.

### Core Architectural Laws of ATOMS Storage

1. **Hardware Transport Isolation**:
   A hardware controller driver (e.g., AHCI, NVMe, USB xHCI) manages only hardware registers, DMA rings, command FIS/queues, and physical device communication. It **MUST NOT** contain filesystem logic, partition parsing, VFS policies, or UI data.
2. **Filesystem Independence**:
   A filesystem driver (e.g., NTFS, FAT32) operates strictly on abstract LBA sectors via the `BlockDevice` interface. It **MUST NOT** perform port I/O, MMIO accesses, or interact directly with PCI or controller hardware.
3. **Single Authoritative Block Device Abstraction**:
   All physical drives, hardware controller ports, partitions, and RAM disks expose the uniform ATOMS `BlockDevice` interface. No subsystem may invent a competing storage abstraction.
4. **Dynamic Discovery (Zero Hardcoding)**:
   All controller bases, port states, drive capacities, sector sizes, partition boundaries, and filesystem identifiers are discovered dynamically at runtime from PCI, IDENTIFY, and disk metadata. No vendor IDs, sizes, or signatures may be hardcoded.
5. **Phase-Isolated Certification**:
   Storage drivers are implemented, validated on real bare metal, certified, and **frozen one controller family at a time**.

---

## 2. Master Storage Tree

```text
                               ATOMS STORAGE
                                    │
                               PCI / BUS
                               DISCOVERY
                                    │
        ┌───────────────────────────┼───────────────────────────┐
        │                           │                           │
        ▼                           ▼                           ▼
    SATA / AHCI                   NVMe                  USB MASS STORAGE
   (Intel 700 / H81)         (PCIe Gen3 / Gen4)              (xHCI)
        │                           │                           │
        ▼                           ▼                           ▼
  SATA SSD / HDD                NVMe M.2 SSD            USB Drive / Pendrive
        │                           │                           │
        └───────────────────────────┼───────────────────────────┘
                                    │
                                    ▼
                         ATOMS BLOCK DEVICE LAYER
                      (`BlockDevice` / `block_device.h`)
                                    │
                                    ▼
                             PARTITION LAYER
                         (`mbr.c` / `mbr.h`)
                      ┌─────────────┴─────────────┐
                      ▼                           ▼
                  MBR Table                   GPT Table
                 (Legacy MBR)            (Protective MBR 0xEE)
                      │                           │
                      └─────────────┬─────────────┘
                                    │
                                    ▼
                            DISK MANAGER LAYER
                  (`disk_manager.c` / `disk_manager.h`)
                                    │
                                    ▼
                        FILESYSTEM DETECTION LAYER
                       (`vfs_detect_fs()` in `vfs.c`)
                      ┌─────────────┴─────────────┐
                      ▼                           ▼
                    NTFS                        FAT32
               (`ntfs.c` / `ntfs.h`)       (`fat32.c` / `fat32.h`)
                      │                           │
                      └─────────────┬─────────────┘
                                    │
                                    ▼
                                ATOMS VFS
                       (`vfs.c` / `vfs_mount.c`)
                                    │
                                    ▼
                           MOUNT / VOLUME TREE
                    `/` (Root), `/volumes/sata0p1`, ...
                                    │
                                    ▼
                           ATOMS FILE MANAGER
                      (Desktop Shell / File Browser)
```

### Extended Controller Roadmap (Future Families)

```text
ATOMS STORAGE EXPANSION
  ├── Legacy IDE / ATA       ──> Vintage ISA/IDE PIO for 486/Pentium legacy compatibility
  ├── RAID / Intel VMD       ──> Volume Management Device passthrough for enterprise NVMe
  ├── SD / eMMC              ──> SDHC / SDXC embedded storage controllers
  ├── SCSI / SAS             ──> Enterprise server storage controllers
  └── VirtIO Storage         ──> `virtio-blk` and `virtio-scsi` para-virtualized drivers
```

---

## 3. Subsystem Boundaries & Invariants

```text
┌─────────────────────────────────────────────────────────────────────────┐
│                           1. PCI / BUS LAYER                            │
│  - Reads PCI Config Space (0xCF8/0xCFC)                                 │
│  - Identifies Class 0x01 (Storage), Subclasses (0x01, 0x04, 0x06, 0x08) │
│  - Configures Command Register (Bus Mastering, MMIO Space)              │
└────────────────────────────────────┬────────────────────────────────────┘
                                     │ Passes PCIDevice*
                                     ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                      2. CONTROLLER DRIVER LAYER                         │
│  - AHCI (`ahci.c`): Maps ABAR (BAR5), handles HBA reset, ports, FIS     │
│  - NVMe (`nvme.c`): Maps BAR0, manages ASQ/ACQ, controller identify     │
│  - ATA (`ata.c`): Legacy ISA PIO ports (0x1F0, 0x170)                   │
│  - Owns DMA buffers (pmm_alloc_pages) and MMIO (vmm_map_page)           │
└────────────────────────────────────┬────────────────────────────────────┘
                                     │ Registers BlockDevice
                                     ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                      3. BLOCK DEVICE LAYER (ATOMS CORE)                 │
│  - Uniform API: `block_device_read()`, `write()`, `flush()`             │
│  - Sector addressing: 64-bit LBA                                        │
│  - Completely hardware-agnostic                                         │
└────────────────────────────────────┬────────────────────────────────────┘
                                     │ Queries Sector 0 / 1
                                     ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                      4. PARTITION & DISK MANAGER LAYER                  │
│  - `mbr_parse()`: Detects MBR signature (0xAA55)                        │
│  - Evaluates GPT Protective MBR (0xEE) -> Reads LBA 1 "EFI PART"        │
│  - Registers Logical Partitions (`disk0p1`, `disk0p2`)                  │
│  - Translates relative partition LBAs to absolute physical disk LBAs    │
└────────────────────────────────────┬────────────────────────────────────┘
                                     │ Passes Logical BlockDevice*
                                     ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                     5. FILESYSTEM AUTO-DETECTION LAYER                  │
│  - Probes Sector 0 of partition for magic signatures:                   │
│    * "NTFS    " at offset 0x03 -> Route to NTFS driver                  │
│    * "FAT32   " at offset 0x52 or BPB markers -> Route to FAT32 driver  │
└────────────────────────────────────┬────────────────────────────────────┘
                                     │ Mounts Driver
                                     ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                          6. VFS MOUNT & NAMESPACE                       │
│  - `vfs_mount_fs(target_path, device_id, fs_type)`                      │
│  - Mount Table links path prefix (e.g. `/volumes/sata0p1`) to driver    │
│  - VFS strips prefix before passing path to filesystem driver           │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 4. Reference Engineering Study

### A. Linux Storage Subsystem Reference
* **Architecture Studied**: Linux `libata`, `drivers/ata/ahci.c`, `drivers/nvme/host/pci.c`, `block/blk-core.c`.
* **What Linux Does**:
  1. Linux uses `pci_driver` registration to bind device IDs to driver probe routines (`ahci_init_one`).
  2. For AHCI, it maps BAR5, issues a controller reset (`GHC.HR`), initializes Ports Implemented (`PI`), and sets up a 32-slot command header list and FIS receive area for each active port.
  3. Commands are encapsulated in an `ata_queued_cmd` and translated to an H2D Register FIS.
  4. Physical memory is mapped into scatter-gather DMA lists (PRD Table).
  5. The generic block layer (`blk-mq`) dispatches requests into hardware submission queues.
* **Why Linux Does It**:
  To achieve asynchronous, multi-queue, high-throughput I/O with maximum hardware parallelism across thousands of heterogeneous storage controllers.
* **What ATOMS Adopts**:
  - The precise AHCI initialization handshake: HBA Reset -> GHC.AE Enable -> BOHC BIOS/OS handoff -> Port Reset -> FIS Enable (`FRE`) -> Command List Start (`ST`).
  - Strict 1024-byte alignment for Command Lists and 256-byte alignment for Received FIS.
* **What ATOMS Avoids**:
  - ATOMS avoids Linux's massive indirection layers (`scsi_host`, `scsi_device`, `libata`, `bio`, `request_queue`). ATOMS connects the hardware controller directly to the clean ATOMS `BlockDevice`.

---

### B. Windows NT Storage Subsystem Reference
* **Architecture Studied**: Windows NT Storage Architecture, Storport Driver (`storport.sys`), Storage Miniport Drivers (`storahci.sys`, `stornvme.sys`), Disk Class Driver (`disk.sys`), Partition Manager (`partmgr.sys`), Volume Manager (`volmgr.sys`).
* **What Windows NT Does**:
  1. Separates hardware access (Miniport) from operating system plumbing (Port Driver). The miniport only handles hardware registers and interrupt service routines.
  2. The Disk Class driver translates file system requests into SCSI Request Blocks (`SRB`) or I/O Request Packets (`IRP`).
  3. `partmgr.sys` inspects raw physical disks, detects MBR or GPT partitioning, and creates Device Objects for each partition (`\Device\Harddisk0\Partition1`).
  4. `volmgr.sys` maps partition device objects into user-accessible volume paths (`\Device\HarddiskVolume1`, `C:`).
* **Why Windows NT Does It**:
  To provide clean binary compatibility for third-party hardware vendors while maintaining a strict, verifiable device object hierarchy.
* **What ATOMS Adopts**:
  - The clean separation between physical storage devices, logical partition objects, and user-facing mount paths.
  - The deterministic initialization hierarchy: Bus ➔ Controller ➔ Physical Disk ➔ Partition Object ➔ Volume Mount.
* **What ATOMS Avoids**:
  - Asynchronous Windows IRP complexity, complex device stack filter drivers, and Registry-dependent storage device naming.

---

## 5. ATOMS-Native Driver Model

ATOMS implements a lightweight, uniform driver contract for all storage hardware:

```c
typedef struct StorageDriver {
    const char* name;
    bool (*probe)(PCIDevice* pci_dev);
    bool (*init)(PCIDevice* pci_dev);
    void (*shutdown)(void);
} StorageDriver;
```

When a controller driver successfully probes and initializes hardware:
1. It allocates contiguous physical DMA frames via `pmm_alloc_pages()`.
2. It maps controller MMIO registers into kernel virtual space via `vmm_map_page()`.
3. It performs hardware identification (IDENTIFY DEVICE or IDENTIFY CONTROLLER).
4. It dynamically constructs and registers a `BlockDevice`:
   ```c
   static BlockDevice s_ahci_bdev = {
       .name = "ahci_disk0",
       .sector_size = 512,
       .sector_count = dynamic_capacity,
       .read_only = true, // Initial bring-up phase
       .driver_data = &s_ahci_port_data,
       .read = ahci_read_sectors,
       .write = NULL,     // Read-only safety invariant
       .flush = NULL
   };
   block_device_register(&s_ahci_bdev);
   ```

---

## 6. Mount & Volume Namespace Hierarchy

ATOMS provides a predictable, human-readable volume namespace:

| Mount Point | Underlying Storage Device | Purpose |
| :--- | :--- | :--- |
| `/` | Primary Root Filesystem | Base OS namespace (Fallback RAM or Primary System Disk) |
| `/volumes/sata0p1` | SATA Disk 0, Partition 1 | Physical SATA SSD/HDD Primary Volume |
| `/volumes/sata0p2` | SATA Disk 0, Partition 2 | Physical SATA Secondary Volume |
| `/volumes/nvme0p1` | NVMe SSD 0, Partition 1 | High-speed NVMe Storage Volume |
| `/volumes/usb0p1` | USB Drive 0, Partition 1 | Removable External Storage Volume |

---

## 7. Driver Status Registry

| Storage Controller Driver | Hardware / Bus Type | Current Status | Validation Target |
| :--- | :--- | :---: | :--- |
| **PCI Storage Discovery** | PCI Configuration (0xCF8/0xCFC) | 🟢 **CERTIFIED** | ASUS B750M-K, QEMU, VMware |
| **Legacy ATA / IDE** | ISA I/O Ports (0x1F0, 0x170) | 🟢 **CERTIFIED** | QEMU IDE Emulation, Haswell H81 |
| **SATA / AHCI** | PCIe MMIO (BAR5 / ABAR) | 🟡 **DESIGN COMPLETE (NEXT IMPLEMENTATION)** | Real SATA SSD & HDD (B750M-K) |
| **NVMe Gen3 / Gen4** | PCIe MMIO (BAR0) | 🔴 **PLANNED (AFTER AHCI CERTIFICATION)** | Real M.2 NVMe SSD (B750M-K) |
| **USB Mass Storage (BOT)**| USB 3.0 / xHCI | 🔴 **PLANNED (AFTER NVMe CERTIFICATION)** | Physical USB Flash Drives |
| **Intel VMD / RAID** | PCIe Class 0x01 Subclass 0x04 | ⚪ **INVESTIGATING TOPOLOGY** | ASUS B750M-K Intel RST |
| **VirtIO-blk** | Para-virtualized PCI | ⚪ **PLANNED (FUTURE)** | Cloud & Hypervisor VMs |

---

## 8. Hardware Compatibility Matrix

| Platform | Chipset / CPU | Primary Storage Controller | Target Driver | Status |
| :--- | :--- | :--- | :--- | :---: |
| **ASUS B750M-K** | Intel B760 / i3-14100F (LGA1700) | Intel 700-series AHCI SATA (0x8086:0x7A62) | Native AHCI | 🟡 Phase 1B Target |
| **ASUS B750M-K** | Intel B760 / i3-14100F (LGA1700) | PCIe M.2 NVMe SSD | Native NVMe | 🔴 Phase 1C Target |
| **Intel H81** | Haswell / i3-4130 (LGA1150) | Intel 8-series AHCI SATA (0x8086:0x8C02) | Native AHCI | 🟡 Compatible |
| **QEMU x86_64** | Standard PC (Q35 Chipset) | `ich9-ahci` (`-device ich9-ahci`) | Native AHCI | 🟢 Pre-Flight Validated |
| **QEMU x86_64** | Standard PC (i440FX Chipset) | `piix3-ide` (`-drive if=ide`) | Legacy ATA | 🟢 Certified |

---

## 9. Protected Subsystems — Zero-Touch Policy

To prevent regressions during storage driver development, the following subsystems are strictly protected under RULE 0:

- **UEFI Bootloader & Kernel Entry**: `boot/`, `kernel/kernel.c` (protected).
- **Core Memory Architecture**: `pmm.c`, `vmm.c` (use existing public APIs only).
- **Certified xHCI & USB HID**: `kernel/drivers/usb/` (do not alter HID or keyboard/mouse state).
- **Certified Window & Display Management**: ABDE, DGL, BCM, ROOK Engine.
- **Certified Syscall Security Boundary**: `validation.c`, `services.c`.
- **Filesystem Internals**: NTFS and FAT32 drivers (read-only consumer boundary).
