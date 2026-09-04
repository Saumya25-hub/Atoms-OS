# ATOMS OS — AHCI DRIVER IMPLEMENTATION DESIGN

**Document**: `docs/drivers/storage/ahci/AHCI_DRIVER_DESIGN.md`  
**Subsystem**: Native Serial ATA AHCI Controller Driver  
**Component Path**: `kernel/drivers/storage/ahci/`  
**Certification Target**: Real SATA SSD & HDD on ASUS B750M-K (Intel 700-series PCH)

---

## 1. High-Level Driver Architecture

The ATOMS AHCI driver is structured into four deterministic functional layers:

```text
┌────────────────────────────────────────────────────────────────────────┐
│                   Layer 4: BlockDevice Adapter Layer                   │
│   Exposes ahci_read_sectors() to ATOMS BlockDevice & Disk Manager      │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│                   Layer 3: ATA Command & Protocol Layer                │
│   Constructs H2D FIS for IDENTIFY (0xEC) and READ DMA EXT (0x25)       │
│   Parses dynamic drive geometry (LBA48 capacity, model, serial)        │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│                 Layer 2: Port & DMA Queue Execution Layer              │
│   Manages Command List (1KB aligned), Received FIS (256B aligned),     │
│   Command Table (128B aligned), PRDTs, and bounded PxCI polling        │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│                 Layer 1: Controller & Bus Management Layer             │
│   PCI scanning, ABAR MMIO page mapping, BIOS/OS handoff, HBA reset     │
└────────────────────────────────────────────────────────────────────────┘
```

---

## 2. Deterministic Initialization Algorithms

### Algorithm 1: Controller Probe & Host Initialization (`ahci_init()`)

```text
1. Iterate PCI devices looking for:
   - dev->base_class == 0x01 (Mass Storage)
   - dev->sub_class == 0x06 (SATA)
   - dev->prog_if == 0x01 (AHCI 1.0)
2. Assert PCI Configuration:
   - pci_enable_memory_space(dev)   -> Set PCI_COMMAND_MEMORY (bit 1)
   - pci_enable_bus_mastering(dev) -> Set PCI_COMMAND_MASTER (bit 2)
3. Retrieve ABAR Physical Address:
   - abar_phys = dev->bars[5].base_address
4. Map ABAR to Virtual MMIO:
   - Map 4KB (or ABAR size) using vmm_map_page(kernel_pml4, abar_phys, abar_virt, VMM_FLAG_WRITABLE)
5. Perform BIOS/OS Handoff (BOHC):
   - Check if (CAP2 & (1 << 0)) [BOH supported]
   - If supported:
       Write BOHC |= (1 << 1) [Set OOS - OS Owned Semaphore]
       Loop with bounded timeout (e.g. 50ms):
           If !(BOHC & (1 << 0)) [BOS released by BIOS]: Break
6. Global HBA Reset:
   - Write GHC |= (1 << 0) [Set HR - HBA Reset]
   - Spin with bounded timeout (max 1,000,000 cycles) waiting for GHC.HR == 0
7. Enable AHCI Mode:
   - Write GHC |= (1 << 31) [Set AE - AHCI Enable]
8. Enumerate Active Ports:
   - Read PI (Ports Implemented) register
   - For port p from 0 to 31:
       If (PI & (1 << p)):
           ahci_init_port(controller, p)
```

---

### Algorithm 2: Port Configuration & Link Detection (`ahci_init_port()`)

```text
1. Stop Port Engine:
   - Write PxCMD &= ~(PxCMD_ST | PxCMD_FRE)
   - Poll until !(PxCMD & (PxCMD_CR | PxCMD_FR)) [Bounded timeout]
2. Allocate Physical DMA Memory Structures:
   - Command List: 1024 bytes (1KB aligned)
   - Received FIS: 256 bytes (256B aligned)
   - Command Table: 256 bytes (128B aligned)
   - Allocate 1 physical 4KB frame via pmm_alloc_page()
   - Partition the frame:
       Offset 0x000: Command List (1024 bytes)
       Offset 0x400: Received FIS (256 bytes)
       Offset 0x500: Command Table for Slot 0 (256 bytes)
   - Zero entire 4KB frame
3. Program Port Address Registers:
   - PxCLB = frame_phys + 0x000
   - PxCLBU = (frame_phys >> 32)
   - PxFB  = frame_phys + 0x400
   - PxFBU = (frame_phys >> 32)
4. Clear Diagnostic Error & Interrupt Status:
   - PxSERR = 0xFFFFFFFF
   - PxIS   = 0xFFFFFFFF
5. Start Port Engine:
   - Write PxCMD |= PxCMD_FRE
   - Write PxCMD |= PxCMD_ST
6. Verify SATA Phy Link State:
   - Read PxSSTS
   - If ((PxSSTS & 0x0F) != 0x03):
       Return FALSE (No physical drive or Phy link down)
7. Verify ATA Device Signature:
   - Read PxSIG
   - If (PxSIG != 0x00000101):
       Return FALSE (Not an ATA disk; e.g. ATAPI optical drive)
8. Issue IDENTIFY DEVICE Command to query geometry.
```

---

### Algorithm 3: ATA Command Execution (`ahci_issue_command()`)

```text
Inputs: Port P, Slot 0, ATA Command, LBA, Sector Count, Physical Buffer Address

1. Clear port interrupt status: PxIS = 0xFFFFFFFF
2. Wait for port not busy:
   - Poll while (PxTFD & (PxTFD_BSY | PxTFD_DRQ)) [Bounded timeout 100,000 cycles]
3. Setup Command Header (Slot 0):
   - cfl = sizeof(AHCIFIS_RegH2D) / 4 = 5 DWORDS
   - w = 0 (Read from device)
   - prdtl = 1 (Single PRD entry)
   - ctba = physical address of Command Table
4. Setup PRD Table Entry:
   - dba = buffer_phys
   - dbc = (sector_count * 512) - 1
   - i = 1 (Interrupt on completion)
5. Setup Command FIS (inside Command Table):
   - fis_type = 0x27 (Register H2D)
   - c = 1 (Command)
   - command = ATA_CMD (0xEC for IDENTIFY, 0x25 for READ DMA EXT)
   - device = (1 << 6) [LBA mode]
   - For LBA48:
       lba0..lba5 = lba & 0xFFFFFFFFFFFF
       count_low = sector_count & 0xFF
       count_high = (sector_count >> 8) & 0xFF
6. Memory Barrier: __asm__ volatile("" ::: "memory")
7. Issue Command:
   - Write PxCI = (1 << 0)
8. Await Completion with Bounded Polling:
   - Loop up to 2,000,000 cycles:
       If !(PxCI & (1 << 0)):
           Command completed successfully -> Break
       If (PxTFD & PxTFD_ERR):
           Hardware error detected -> Return FALSE
       pause
9. Return TRUE
```

---

## 3. Dynamic Drive Discovery (IDENTIFY Parsing)

ATOMS extracts all physical disk metrics dynamically from the 512-byte (256-word) IDENTIFY DEVICE buffer:

| Field | IDENTIFY Words | Extraction Logic | Safety Invariant |
| :--- | :---: | :--- | :--- |
| **Model String** | Words 27–46 | 40 ASCII characters. Bytes within each word are byte-swapped. | Null-terminate & trim trailing spaces. |
| **Serial Number**| Words 10–19 | 20 ASCII characters. Byte-swapped. | Null-terminate. |
| **LBA48 Support** | Word 83 bit 10 | If set, device supports 48-bit LBA (> 128 GB capacity). | Mandatory check for drives > 137 GB. |
| **LBA48 Sectors** | Words 100–103| 64-bit integer: `W100 \| (W101 << 16) \| (W102 << 32) \| (W103 << 48)` | True physical capacity. |
| **LBA28 Sectors** | Words 60–61 | 32-bit integer: `W60 \| (W61 << 16)` | Fallback capacity if LBA48 is not supported. |
| **Sector Size** | Words 106 & 117 | Word 106 bit 12: non-512B flag. Words 117–118: logical sector size. | Default to 512 bytes if legacy format. |

---

## 4. BlockDevice Integration

Upon successful IDENTIFY parsing, the driver binds to the ATOMS storage ecosystem:

```c
typedef struct {
    AHCIController* controller;
    uint8_t port_num;
    uint64_t total_sectors;
    uint32_t sector_size;
    char model[41];
    char serial[21];
} AHCIDriveContext;

static bool ahci_read_sectors_adapter(BlockDevice* dev, uint64_t lba, uint32_t count, void* buffer) {
    AHCIDriveContext* ctx = (AHCIDriveContext*)dev->driver_data;
    return ahci_read_sectors(ctx->controller, ctx->port_num, lba, count, buffer);
}
```

The `BlockDevice` is registered with:
- `dev->name = "sata_disk0"` (or dynamically `sata_diskX`)
- `dev->sector_size = ctx->sector_size` (512)
- `dev->sector_count = ctx->total_sectors`
- `dev->read_only = true` (Read-only enforcement during Phase 1B)
- `dev->read = ahci_read_sectors_adapter`
- `dev->write = NULL`
- `dev->flush = NULL`

`block_device_register(dev)` registers the device in the global ATOMS block registry, where `disk_manager_init()` immediately scans it for MBR and GPT partition tables.
