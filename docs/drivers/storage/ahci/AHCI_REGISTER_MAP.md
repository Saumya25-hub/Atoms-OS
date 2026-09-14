# ATOMS OS — AHCI REGISTER MAP & DATA STRUCTURE SPECIFICATION

**Document**: `docs/drivers/storage/ahci/AHCI_REGISTER_MAP.md`  
**Specification Baseline**: Serial ATA Advanced Host Controller Interface (AHCI) 1.3.1  
**Subsystem**: Storage Controller Hardware Register Specification

---

## 1. Global HBA Memory-Mapped Registers (Generic Host Control)

The AHCI Base Address Register (PCI BAR5 / ABAR) points to the start of the AHCI MMIO space. The global registers reside from offset `0x00` to `0x2B`.

| Offset | Name | Type | Reset | Description |
| :---: | :--- | :---: | :---: | :--- |
| `0x00` | **CAP** | RO | Varies | **Host Capabilities**: Max ports (`NP`), Command slots (`NCS`), 64-bit addressing (`S64A`), Staggered Spin-up (`SSS`). |
| `0x04` | **GHC** | RW | `0x00000000` | **Global Host Control**: `AE` (AHCI Enable, bit 31), `IE` (Interrupt Enable, bit 1), `HR` (HBA Reset, bit 0). |
| `0x08` | **IS** | RWC | `0x00000000` | **Interrupt Status**: Bitmask of pending port interrupts (bits 0–31). |
| `0x0C` | **PI** | RO | Varies | **Ports Implemented**: Bitmask indicating which of the 32 ports are physically connected to hardware. |
| `0x10` | **VS** | RO | Varies | **AHCI Version**: Major (bits 31:16), Minor (bits 15:0). E.g., `0x00010301` = v1.3.1. |
| `0x14` | **CCC_CTL** | RW | `0x00000000` | Command Completion Coalescing Control. |
| `0x18` | **CCC_PORTS**| RW | `0x00000000` | Command Completion Coalescing Ports. |
| `0x1C` | **EM_LOC** | RO | Varies | Enclosure Management Location. |
| `0x20` | **EM_CTL** | RW | `0x00000000` | Enclosure Management Control. |
| `0x24` | **CAP2** | RO | Varies | **Extended Host Capabilities**: `BOH` (BIOS/OS Handoff supported, bit 0), `NVMP` (NVMHCI present). |
| `0x28` | **BOHC** | RW | `0x00000000` | **BIOS/OS Handoff Control and Status**: `OOS` (OS Owned Semaphore, bit 1), `BOS` (BIOS Owned Semaphore, bit 0). |

### Global Register Bitfields & Masks
- `GHC_HR`  = `(1U << 0)`  — HBA Reset. When written to 1, controller resets all internal logic. Cleared by hardware on completion.
- `GHC_IE`  = `(1U << 1)`  — Global Interrupt Enable.
- `GHC_AE`  = `(1U << 31)` — AHCI Enable. Must be set to 1 before any port registers are accessed.
- `BOHC_BOS`= `(1U << 0)`  — BIOS Owned Semaphore.
- `BOHC_OOS`= `(1U << 1)`  — OS Owned Semaphore. Writing 1 requests OS ownership from UEFI/BIOS.

---

## 2. Port-Specific Registers

Each port occupies `0x80` bytes of register space. The register set for Port `P` starts at offset:
$$\text{PortOffset}(P) = 0x100 + (P \times 0x80)$$

| Offset | Name | Type | Description |
| :---: | :--- | :---: | :--- |
| `+0x00` | **PxCLB** | RW | **Port Command List Base Address (Bits 31:10)**. Must be 1024-byte aligned. |
| `+0x04` | **PxCLBU**| RW | **Port Command List Base Address Upper 32 Bits**. (For 64-bit DMA). |
| `+0x08` | **PxFB** | RW | **Port FIS Base Address (Bits 31:8)**. Must be 256-byte aligned. |
| `+0x0C` | **PxFBU** | RW | **Port FIS Base Address Upper 32 Bits**. |
| `+0x10` | **PxIS** | RWC | **Port Interrupt Status**: DHRS, PSS, DSS, SDBS, UFS, IFS, PRCS, OFS, INFS, IFS, HBDS, HBFS, TFES, CPDS. |
| `+0x14` | **PxIE** | RW | **Port Interrupt Enable**: Controls which PxIS bits trigger an interrupt. |
| `+0x18` | **PxCMD** | RW | **Port Command and Status**: Controls execution state machines (`ST`, `FRE`, `FR`, `CR`). |
| `+0x20` | **PxTFD** | RO | **Task File Data**: Reflects ATA Task File status (`BSY`, `DRQ`, `ERR`). |
| `+0x24` | **PxSIG** | RO | **Signature**: Contains 32-bit hardware device signature after COMRESET. |
| `+0x28` | **PxSSTS**| RO | **Serial ATA Status**: Phy link detection (`DET`), power management (`IPM`), speed (`SPD`). |
| `+0x2C` | **PxSCTL**| RW | **Serial ATA Control**: Phy link reset and initialization (`DET`). |
| `+0x30` | **PxSERR**| RWC | **Serial ATA Error**: Diagnostic error bits (must be cleared by writing 1s). |
| `+0x34` | **PxSACT**| RW | **Serial ATA Active**: Native Command Queuing (NCQ) issue register. |
| `+0x38` | **PxCI** | RW | **Command Issue**: Bitmask of command slots issued to hardware. Hardware clears bits when finished. |

### Critical Port Register Bitfields
- **PxCMD**:
  - `PxCMD_ST`  = `(1U << 0)`  — Start. Setting to 1 enables the command list processing engine.
  - `PxCMD_SUD` = `(1U << 1)`  — Spin-Up Device.
  - `PxCMD_POD` = `(1U << 2)`  — Power On Device.
  - `PxCMD_FRE` = `(1U << 4)`  — FIS Receive Enable. Must be set to 1 before `ST` is set.
  - `PxCMD_FR`  = `(1U << 14)` — FIS Receive Running.
  - `PxCMD_CR`  = `(1U << 15)` — Command List Running.
- **PxTFD**:
  - `PxTFD_ERR` = `(1U << 0)`  — Error bit in Status register.
  - `PxTFD_DRQ` = `(1U << 3)`  — Data Request bit.
  - `PxTFD_BSY` = `(1U << 7)`  — Busy bit.
- **PxSSTS**:
  - `PxSSTS_DET_MASK` = `0x0F`
  - `PxSSTS_DET_PRESENT` = `0x03` — Device presence detected and Phy communication established.
  - `PxSSTS_IPM_ACTIVE`  = `0x01` — Interface in active power management state.
- **PxSIG (Device Signature)**:
  - `0x00000101`: SATA Drive (SATA HDD or SATA SSD).
  - `0xEB140101`: SATAPI Drive (Optical CD/DVD Drive).
  - `0xC33C0101`: Enclosure Management Bridge (SEMB).
  - `0x96690101`: Port Multiplier.

---

## 3. Host Memory Structures & DMA Layout

All AHCI memory structures must be physically contiguous and aligned in host RAM.

### A. Port Command List (Allocated at `PxCLB`)
An array of 32 **Command Headers** (each 32 bytes). Total size = **1024 bytes** (Alignment: 1024 bytes).

```c
typedef struct __attribute__((packed)) {
    // DW0
    uint8_t  cfl:5;         // Command FIS Length in DWORDS (e.g. 5 for RegH2D)
    uint8_t  a:1;           // ATAPI
    uint8_t  w:1;           // Write (1 = Write to Device, 0 = Read from Device)
    uint8_t  p:1;           // Prefetchable
    uint8_t  r:1;           // Reset
    uint8_t  b:1;           // BIST
    uint8_t  c:1;           // Clear Busy upon R_OK
    uint8_t  rsvd0:1;
    uint8_t  pmp:4;         // Port Multiplier Port
    uint16_t prdtl;         // Physical Region Descriptor Table Length (number of entries)

    // DW1
    volatile uint32_t prdbc;// PRD Byte Count transferred

    // DW2, DW3
    uint32_t ctba;          // Command Table Descriptor Base Address (Bits 31:7, 128B aligned)
    uint32_t ctbau;         // Command Table Descriptor Base Address Upper 32 Bits

    // DW4 - DW7
    uint32_t rsvd1[4];      // Reserved
} AHCICommandHeader;
```

---

### B. Received FIS Area (Allocated at `PxFB`)
Total size = **256 bytes** (Alignment: 256 bytes).

```c
typedef struct __attribute__((packed)) {
    uint8_t dsfis[0x1C];    // DMA Setup FIS
    uint8_t rsvd0[0x04];
    uint8_t psfis[0x14];    // PIO Setup FIS
    uint8_t rsvd1[0x0C];
    uint8_t rfis[0x14];     // D2H Register FIS
    uint8_t rsvd2[0x04];
    uint8_t sdbfis[0x08];   // Set Device Bits FIS
    uint8_t ufis[0x40];     // Unknown FIS
    uint8_t rsvd3[0x60];    // Reserved
} AHCIFISReceived;
```

---

### C. Command Table (Pointed to by `ctba` in Command Header)
Total size = $128 \text{ bytes} + (\text{PRDTL} \times 16 \text{ bytes})$. (Alignment: 128 bytes).

```c
typedef struct __attribute__((packed)) {
    // 0x00 - 0x3F: Command FIS (up to 64 bytes)
    uint8_t  cfis[64];

    // 0x40 - 0x4F: ATAPI Command (16 bytes)
    uint8_t  acmd[16];

    // 0x50 - 0x7F: Reserved (48 bytes)
    uint8_t  rsvd[48];

    // 0x80+: Physical Region Descriptor Table (PRDT) entries
    AHCIPRDTEntry prdt_entries[1]; // Flexible array (size defined by prdtl)
} AHCICommandTable;
```

---

### D. Physical Region Descriptor Table (PRDT Entry)
Each entry describes a contiguous chunk of physical memory for DMA data transfer (16 bytes).

```c
typedef struct __attribute__((packed)) {
    uint32_t dba;           // Data Base Address (Bits 31:1, Bit 0 must be 0)
    uint32_t dbau;          // Data Base Address Upper 32 Bits
    uint32_t rsvd0;         // Reserved
    uint32_t dbc:22;        // Data Byte Count (0-indexed: value is count - 1)
    uint32_t rsvd1:9;       // Reserved
    uint32_t i:1;           // Interrupt on Completion (1 = trigger interrupt)
} AHCIPRDTEntry;
```

---

### E. Host-to-Device Register FIS (FIS Type 0x27)
Standard 20-byte FIS used to issue ATA commands (IDENTIFY, READ DMA EXT, etc.).

```c
typedef struct __attribute__((packed)) {
    uint8_t  fis_type;      // FIS_TYPE_REG_H2D = 0x27
    uint8_t  pm:4;          // Port multiplier
    uint8_t  rsvd0:3;       // Reserved
    uint8_t  c:1;           // Command: 1 = Command Register write, 0 = Control Register write
    uint8_t  command;       // ATA Command (0xEC = IDENTIFY, 0x25 = READ DMA EXT)
    uint8_t  feature_low;   // Feature Low byte

    uint8_t  lba0;          // LBA Bits 7:0
    uint8_t  lba1;          // LBA Bits 15:8
    uint8_t  lba2;          // LBA Bits 23:16
    uint8_t  device;        // Device Register (bit 6 = 1 for LBA mode)

    uint8_t  lba3;          // LBA Bits 31:24
    uint8_t  lba4;          // LBA Bits 39:32
    uint8_t  lba5;          // LBA Bits 47:40
    uint8_t  feature_high;  // Feature High byte

    uint8_t  count_low;     // Sector Count Bits 7:0
    uint8_t  count_high;    // Sector Count Bits 15:8
    uint8_t  icc;           // Isochronous Command Completion
    uint8_t  control;       // Control Register

    uint8_t  rsvd1[4];      // Reserved
} AHCIFIS_RegH2D;
```
