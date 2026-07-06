# BSPE Swapchain Subsystem (`Swapchain/`)

> **Module:** BOS Surface Presentation Engine — Swapchain Manager  
> **Status:** Step 2 Empty Production Module (Zero Logic)  

---

## 1. Responsibility
The `Swapchain/` subsystem manages double and triple buffering state rotation. It tracks buffer ownership between front (displaying), back (rendering), and staging buffers, ensuring zero visual tearing and zero race conditions during VRAM page flips.

## 2. Dependencies
* **Upstream:** Controlled by `BSPE/Present/` (Present Queue) and `BSPE/FramePacer/` (VSync Timer).
* **Downstream:** Instructs `BSPE/DisplayHAL/` to execute hardware page flips.
* **Allowed Includes:** `bspe.h`, `display_hal.h`.
* **Forbidden Includes:** Video drivers (`vbe.h`), BOGE internal drawing headers, shell/UI headers.
