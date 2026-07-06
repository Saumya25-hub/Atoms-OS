# BSPE Video Driver Backends (`Drivers/`)

> **Module:** BOS Surface Presentation Engine — Hardware Video Drivers  
> **Status:** Step 2 Empty Production Module (Zero Logic)  

---

## 1. Responsibility
The `Drivers/` subsystem contains the physical hardware driver backends that implement the `BSPE_DisplayDriver` interface table. In Phase 1, it houses `vbe_driver.c`, which binds Bochs VBE, VGA I/O ports (`0x01CE`/`0x01CF`, `0x03D4`/`0x03D5`), and VESA Linear Framebuffers (LFB) into BSPE. Future phases will add VirtIO, Intel, and AMD GPU driver backends here.

## 2. Dependencies
* **Upstream:** Controlled exclusively by `BSPE/DisplayHAL/`.
* **Downstream:** Directly executes x86 assembly I/O port instructions (`inb`/`outb`, `inw`/`outw`) and writes to physical MMIO VRAM addresses.
* **Allowed Includes:** `bspe.h`, `display_hal.h`, `vbe.h`.
* **Forbidden Includes:** All higher-level OS, window manager, BOGE, and application headers.
