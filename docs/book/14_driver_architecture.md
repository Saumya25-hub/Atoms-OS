# Chapter 14: Unified Driver Architecture

The BOS Kernel implements a modular hardware driver architecture organized under [`drivers/`](file:///D:/Signatures_OS/drivers) and [`kernel/drivers/`](file:///D:/Signatures_OS/kernel/drivers).

## 1. Device Discovery & PCI Enumeration
During early boot, the PCI manager performs a recursive scan across all 256 PCI buses, 32 devices per bus, and 8 functions per device:
- Reads Vendor ID, Device ID, Class Code, Subclass, and Header Type.
- Maps Base Address Registers (BAR0..BAR5) into kernel virtual MMIO space via `vmm_map_mmio()`.
- Configures PCI Command register (Bus Master Enable, Memory Space Enable, I/O Space Enable).

## 2. Driver Binding Table
- **Class `0x0C` Subclass `0x03` ProgIF `0x30`**: USB 3.0 xHCI Host Controller.
- **Class `0x01` Subclass `0x08` ProgIF `0x02`**: NVM Express (NVMe) Controller.
- **Class `0x01` Subclass `0x06` ProgIF `0x01`**: Serial ATA AHCI 1.0 Controller.
- **Class `0x02` Subclass `0x00`**: Ethernet Controller (Realtek RTL8168 / RTL8111).
- **Class `0x04` Subclass `0x03`**: High Definition Audio (Intel HDA / Realtek ALC662).
