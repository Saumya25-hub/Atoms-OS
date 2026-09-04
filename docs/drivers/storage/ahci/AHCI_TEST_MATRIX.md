# ATOMS OS — AHCI DRIVER VERIFICATION & TEST MATRIX

**Document**: `docs/drivers/storage/ahci/AHCI_TEST_MATRIX.md`  
**Subsystem**: Native Serial ATA AHCI Controller Driver  
**Target Hardware**: ASUS B750M-K (Intel Core i3-14100F LGA1700), QEMU x86_64 UEFI  
**Certification Scope**: Read-Only Physical SATA SSD & SATA HDD Bring-Up

---

## 1. Test Environments

| Parameter | Environment 1: QEMU Pre-Flight | Environment 2: Bare-Metal Real Target |
| :--- | :--- | :--- |
| **System** | QEMU x86_64 UEFI (OVMF) | Physical ASUS B750M-K Motherboard |
| **Chipset** | Intel Q35 Chipset (`-M q35`) | Intel B760 Chipset (LGA1700) |
| **CPU** | QEMU Virtual CPU (4 Cores) | Intel Core i3-14100F (4C/8T 3.5GHz) |
| **AHCI Controller**| `ich9-ahci` (`-device ich9-ahci,id=ahci`) | Intel 700-series SATA AHCI Controller (`0x8086:0x7A62`) |
| **Connected Storage**| QEMU Raw Disk Image (GPT + FAT32/NTFS) | Real 128 GB SATA SSD + 512 GB SATA HDD |
| **Telemetry** | COM1 Serial + Framebuffer capture | Realtek NIC UDP Telemetry Stream (1080p 32bpp) |

---

## 2. Comprehensive Test Cases & Results (TC-AHCI-001 through TC-AHCI-012)

| Test ID | Test Name | Description / Procedure | Expected Result | QEMU Pre-Flight Result | Bare-Metal Target Result |
| :---: | :--- | :--- | :--- | :---: | :---: |
| **TC-AHCI-001** | **PCI Controller Probe** | Scan PCI bus for Class `0x01`, Subclass `0x06`, Prog-IF `0x01`. Read BAR5. | Discovers controller; BAR5 is valid 32/64-bit MMIO. | 🟢 PASS (PCI 0:3.0, Vendor 0x8086, Dev 0x2922) | PENDING B750M-K |
| **TC-AHCI-002** | **PCI Command Assertion** | Verify Bus Master (`0x04`) and Memory Space (`0x02`) bits are asserted in Command register. | Controller acknowledges Bus Master DMA and MMIO read/writes. | 🟢 PASS (Command register enabled) | PENDING B750M-K |
| **TC-AHCI-003** | **ABAR MMIO Mapping** | Map physical BAR5 address into kernel virtual address space via `vmm_map_page()`. | Kernel page table contains valid PTE for ABAR with writable flag. | 🟢 PASS (ABAR 0x81060000 mapped) | PENDING B750M-K |
| **TC-AHCI-004** | **BIOS/OS Handoff** | If `CAP2.BOH` is set, write `BOHC.OOS = 1`. Poll for `BOHC.BOS == 0`. | BIOS releases ownership to OS within 50ms bounded timeout. | 🟢 PASS (BOH checked, ownership granted) | PENDING B750M-K |
| **TC-AHCI-005** | **Global HBA Reset & AE**| Assert `GHC.HR = 1`. Wait for hardware clear. Assert `GHC.AE = 1`. | HBA resets internal logic; locks into native AHCI mode. | 🟢 PASS (GHC=0x80000000, Reset complete) | PENDING B750M-K |
| **TC-AHCI-006** | **Port Link Detection** | Inspect `PI` register. For each set bit, stop port, allocate DMA buffers, restart, check `PxSSTS.DET`. | Active physical ports report Phy link established (`DET == 3`). | 🟢 PASS (Port 0 Link UP, SSTS=0x113) | PENDING B750M-K |
| **TC-AHCI-007** | **Device Signature Check**| Inspect `PxSIG` after link establishment. | SATA storage devices report signature `0x00000101`. | 🟢 PASS (Port 0 SIG=0x00000101) | PENDING B750M-K |
| **TC-AHCI-008** | **IDENTIFY Command** | Issue ATA IDENTIFY (`0xEC`) FIS via Slot 0. Bounded poll on `PxCI`. | Controller transfers 512 bytes of drive parameters to host DMA frame. | 🟢 PASS (PxCI cleared, TFD clean) | PENDING B750M-K |
| **TC-AHCI-009** | **Dynamic Capacity Parse**| Extract LBA48 sector count from IDENTIFY words 100–103. | Reports real dynamic capacity with zero hardcoding. | 🟢 PASS (Model: QEMU HARDDISK, 512MB, 1048576 sec) | PENDING B750M-K |
| **TC-AHCI-010** | **Single Sector DMA Read**| Read LBA 0 (MBR / Protective MBR) using ATA READ DMA EXT (`0x25`). | Sector 0 buffer contains valid MBR boot signature (`0xAA55` at offset 510). | 🟢 PASS (MBR Signature 0xAA55 verified) | PENDING B750M-K |
| **TC-AHCI-011** | **Multi-Sector DMA Read** | Read LBA 1 (GPT Header) and LBA 2–33 (GPT Partition Entries). | Sector 1 contains `"EFI PART"`. Parses partition array. | 🟢 PASS (GPT Header verified, disk0p1 registered) | PENDING B750M-K |
| **TC-AHCI-012** | **VFS Mount & Root Probe**| Auto-detect filesystem (NTFS/FAT32) on partition 1. Mount read-only at `/volumes/...`. | `vfs_readdir()` enumerates real files on disk without errors. | 🟢 PASS (Mounted /volumes/fat32_0, 4 files found) | PENDING B750M-K |

---

## 3. Hardware Pass/Fail Criteria for Certification

The AHCI driver will receive the official **`CERTIFIED`** verdict if and only if:

1. **Clean Controller Reset**: `GHC.HR` completes and `GHC.AE` is asserted cleanly.
2. **Dynamic Hardware Identification**: Discovers the real physical SATA drive without hardcoded metrics.
3. **Valid DMA Sector Transfer**: Reads Sector 0 with valid `0xAA55` signature via hardware DMA.
4. **Partition Layer Handshake**: GPT or MBR partitions are registered in `disk_manager`.
5. **Real Filesystem Reached**: The real partition is recognized as NTFS or FAT32, mounted to `/volumes/sata0p1`, and directory entries are enumerated.
6. **Zero Fallback Reliance**: The dashboard displays `REAL STORAGE: DETECTED & ACTIVE` instead of falling back to `dummyfs`.
7. **Heartbeat & Screen Stream Active**: Rotating spinner active (`| / - \`); full UDP frame received by diagnostic server.
