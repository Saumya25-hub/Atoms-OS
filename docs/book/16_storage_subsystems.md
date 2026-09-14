# Chapter 16: NVMe & AHCI Storage Subsystems

ATOMS OS provides high-throughput storage access across modern NVMe solid-state drives and legacy SATA controllers.

## 1. NVM Express (NVMe Gen4) Driver
- **Queues**: Implements lock-free Admin Submission/Completion Queues and I/O Submission/Completion Queues in host RAM.
- **Doorbell Registers**: Ring buffer head/tail doorbells are updated via direct MMIO writes.
- **DMA Physical Region Pages (PRP)**: Transfers 4 KB data blocks directly between storage media and kernel RAM using zero-copy DMA.
- **Certified Hardware**: Formally verified on physical Western Digital Blue SN5000 NVMe Gen4 SSD on the ASUS B760M-K platform.

## 2. AHCI 1.0 SATA Driver
- Configures the Host Bus Adapter (HBA) into AHCI mode (`GHC.AE = 1`).
- Implements Command List, Received FIS buffers, and Command Tables with Physical Region Descriptor Tables (PRDT).
- Issues ATA READ DMA EXT and WRITE DMA EXT commands for 48-bit LBA disk addressing.
