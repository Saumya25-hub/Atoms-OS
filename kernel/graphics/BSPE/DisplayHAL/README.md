# BSPE Display Hardware Abstraction Layer (`DisplayHAL/`)

> **Module:** BOS Surface Presentation Engine — Display HAL  
> **Status:** Step 2 Empty Production Module (Zero Logic)  

---

## 1. Responsibility
The `DisplayHAL/` subsystem defines the standardized interface table (`BSPE_DisplayDriver`) that decouples presentation logic from physical display controller hardware. It routes swapchain flips, VRAM copying, and cursor register updates to the active hardware driver backend.

## 2. Dependencies
* **Upstream:** Called by `BSPE/Present/`, `BSPE/Swapchain/`, `BSPE/Damage/`, and `BSPE/Cursor/`.
* **Downstream:** Delegates execution to registered backends in `BSPE/Drivers/` (`vbe_driver.c`, future GPU drivers).
* **Allowed Includes:** `bspe.h`, `vbe_driver.h`.
* **Forbidden Includes:** Userspace applications, BOGE V2 rendering loops, window manager internals.
