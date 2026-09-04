# ATOMS OS — AHCI DRIVER ARCHITECTURE SPECIFICATION

**Driver Identifier**: `ahci`  
**Subsystem**: Native Serial ATA Storage Controller  
**Document**: `docs/drivers/storage/ahci/AHCI_ARCHITECTURE.md`  
**Target Hardware Baseline**: Intel 700-series (ASUS B750M-K LGA1700), Intel 8-series (H81 Haswell), QEMU `ich9-ahci`  
**Certification Status**: 🟡 **DESIGN COMPLETE — PRE-IMPLEMENTATION PHASE**

---

## 1. Architectural Overview & Objective

The **Advanced Host Controller Interface (AHCI)** is the industry-standard hardware mechanism defining the interface between system software and Serial ATA (SATA) host controllers. It replaces vintage ISA/IDE legacy PIO with high-throughput **Direct Memory Access (DMA)**, scatter-gather physical region descriptor tables (**PRDT**), and command list queues.

The primary objective of the ATOMS AHCI driver is to discover, initialize, and operate physical SATA SSDs and HDDs attached to motherboard SATA ports in native UEFI mode, exposing them as clean, read-only ATOMS [`BlockDevice`](file:///d:/Signatures_OS/kernel/vfs/vfs_legacy/storage/include/block_device.h) instances.

---

## 2. Hardware Topology & System Integration

```text
┌────────────────────────────────────────────────────────────────────────┐
│                        PCI Express Host Bridge                         │
│             (ASUS B750M-K / Intel 700 Series PCH / Haswell)            │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│                       PCI Configuration Header                         │
│  - Vendor: 0x8086 (Intel)                                              │
│  - Class: 0x01 (Mass Storage) | Subclass: 0x06 (SATA) | Prog-IF: 0x01   │
│  - BAR5: AHCI Base Address Register (ABAR)                             │
│  - Command Register: Enable Memory Space (bit 1) & Bus Master (bit 2)  │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │ Maps ABAR to Virtual MMIO
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│                      Global AHCI HBA Controller                        │
│  - Generic Host Control Registers: CAP, GHC, IS, PI, VS, BOHC          │
│  - Global Reset: GHC.HR                                                │
│  - AHCI Mode Enable: GHC.AE                                            │
│  - Active Port Discovery via Ports Implemented (PI) bitmap             │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │ Up to 32 Port Register Sets
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│                       Individual SATA Port Logic                       │
│  - Port Registers: PxCLB, PxFB, PxIS, PxIE, PxCMD, PxTFD, PxSIG, PxSSTS│
│  - DMA Control Structures (in Host RAM):                               │
│    * Command List (32 slots × 32 bytes = 1024 bytes, 1KB aligned)      │
│    * Received FIS Buffer (256 bytes, 256B aligned)                     │
│    * Command Table (CFIS + ACMD + PRD Table, 128B aligned)             │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │ SATA Phy Link (1.5 / 3.0 / 6.0 Gbps)
                                    ▼
┌───────────────────────────────────┴────────────────────────────────────┐
│                       Physical SATA Storage Device                     │
│  - Device 0: SATA SSD (e.g., 128 GB OS Disk)                           │
│  - Device 1: SATA HDD (e.g., 512 GB Data Disk)                         │
└────────────────────────────────────────────────────────────────────────┘
```

---

## 3. Engineering Reference Studies

### A. Linux `libata` & `ahci.c` Reference Study
* **Reference Implementation**: Linux kernel `drivers/ata/ahci.c` and `drivers/ata/libata-core.c`.
* **Mechanisms Studied**:
  1. **PCI Binding**: In `ahci_init_one()`, Linux reads PCI BAR5, enables bus mastering via `pci_set_master()`, and configures 64-bit or 32-bit DMA masks.
  2. **BIOS/OS Handoff**: Checks `CAP2.BOH` (BIOS/OS Handoff supported). If present, it writes `BOHC.OOS = 1` (OS Owned Semaphore) and polls for `BOHC.BOS == 0` (BIOS Owned Semaphore released) with a 2-second timeout.
  3. **HBA Reset & AE**: Writes `GHC.HR = 1` to reset controller state machines, then asserts `GHC.AE = 1` to lock the controller into native AHCI mode.
  4. **Port Allocation**: For every bit set in the `PI` (Ports Implemented) register, Linux allocates memory for the Command List (1024B aligned) and Received FIS buffer (256B aligned).
  5. **Link Detection**: Checks `PxSSTS.DET == 3` (device detected and Phy communication established) and `PxSSTS.IPM == 1` (interface in active power state).
  6. **Signature Verification**: Reads `PxSIG` to classify device: `0x00000101` = ATA drive (SATA HDD/SSD), `0xEB140101` = ATAPI drive (CD/DVD), `0xC33C0101` = Enclosure management.
  7. **Command Dispatch**: Constructs a Host-to-Device Register FIS (Type `0x27`) with command byte `0xEC` (IDENTIFY DEVICE) or `0x25` (READ DMA EXT), attaches a Physical Region Descriptor Table (PRDT) entry with physical memory addresses, sets `PxCI` (Command Issue), and awaits completion.
* **ATOMS-Native Adaptation**:
  ATOMS adopts the identical hardware register sequence, handoff protocol, alignment invariants, and FIS structure, but implements it as a clean, synchronous, bounded polled engine for the initial phase, avoiding Linux's complex multi-layer `scsi_host` wrapper.

---

### B. Windows NT `storahci.sys` Reference Study
* **Reference Implementation**: Windows NT Storage Architecture, Storport Miniport Model (`storahci.sys`).
* **Mechanisms Studied**:
  1. **Miniport Isolation**: `storahci.sys` implements only hardware access routines (`HwFindAdapter`, `HwInitialize`, `HwStartIo`, `HwInterrupt`). It has zero knowledge of filesystem structures, NTFS attributes, or partition schemes.
  2. **SCSI Pass-Through**: Windows sends SCSI Request Blocks (`SRBs`) to the miniport, where the miniport translates SCSI Read(10/16) into ATA READ DMA EXT FIS commands.
  3. **Strict Reset Hierarchy**: Port-level resets (`COMRESET` via `PxSCTL.DET = 1`) are attempted before resorting to global HBA resets (`GHC.HR = 1`).
* **ATOMS-Native Adaptation**:
  ATOMS maintains the same strict boundary: the AHCI driver knows nothing about partitions or filesystems; it receives LBA sector read requests and completes them via DMA. However, ATOMS bypasses SCSI translation, talking directly to ATA via native FIS commands for maximum performance and code clarity.

---

## 4. ATOMS-Native AHCI Model

The ATOMS AHCI driver operates as follows:

```c
// Primary Controller Structure
typedef struct {
    PCIDevice* pci_dev;
    uintptr_t mmio_base;      // Virtual mapped address of ABAR
    uint32_t ports_implemented;
    uint32_t active_port_count;
    AHCIPort ports[32];
} AHCIController;
```

### Driver Invariants
1. **Zero Dynamic Allocation in Critical Path**: Command headers and FIS buffers are allocated once during port initialization using contiguous physical frames from `pmm_alloc_pages()`.
2. **Deterministic Bounded Timeouts**: Every hardware polling loop (`PxCI`, `PxTFD`, `GHC.HR`, `BOHC`) uses a bounded cycle count with `pause` instructions. No infinite `while(1)` loops.
3. **Strict Memory Ordering**: Volatile register access and memory fences (`__asm__ volatile("" ::: "memory")`) are enforced between PRDT setup and `PxCI` command issue.
4. **Read-Only Safety**: The initial implementation exposes `dev->write = NULL` and `dev->read_only = true`. No physical sectors may be overwritten.

---

## 5. Subsystem File Boundaries

### Files That AHCI Driver Will Create / Touch
- `kernel/drivers/storage/ahci/ahci.c` [NEW]: Native AHCI driver implementation.
- `kernel/drivers/storage/ahci/ahci.h` [NEW]: AHCI register structures and prototypes.
- `kernel/core/pci/pci.c` [TOUCH]: Enable Memory Space and Bus Master for Storage Class `0x01`.
- `kernel/vfs/vfs_legacy/storage/src/disk_manager.c` [TOUCH]: Call `ahci_init()` to register discovered drives alongside legacy ATA.
- `kernel/debug/storage_forensic_debug.c` [TOUCH]: Add Subclass `0x04` string label and update fallback verdict logic.

### Files That AHCI Driver MUST NOT Touch
- ❌ `kernel/vfs/vfs_legacy/fs/ntfs/` (NTFS driver is protected).
- ❌ `kernel/vfs/vfs_legacy/fs/fat32/` (FAT32 driver is protected).
- ❌ `kernel/vfs/vfs_legacy/src/vfs.c` (Core VFS logic is certified).
- ❌ `kernel/core/memory/pmm/` & `kernel/core/memory/vmm/` (Use existing public APIs).
- ❌ `kernel/drivers/usb/` (Certified xHCI HID mouse/keyboard must remain untouched).
- ❌ `kernel/kernel.c` (Kernel entry is protected).
