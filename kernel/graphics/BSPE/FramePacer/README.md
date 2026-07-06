# BSPE Frame Pacer Subsystem (`FramePacer/`)

> **Module:** BOS Surface Presentation Engine — VSync Frame Pacer  
> **Status:** Step 2 Empty Production Module (Zero Logic)  

---

## 1. Responsibility
The `FramePacer/` subsystem synchronizes presentation page flips with monitor Vertical Blanking Intervals (VBI / VSync IRQs). It prevents CPU overruns, eliminates frame tearing, and regulates frame presentation intervals (60 Hz / 144 Hz).

## 2. Dependencies
* **Upstream:** Receives synchronization intervals from `BSPE_SetSwapInterval()`.
* **Downstream:** Triggers `BSPE/Swapchain/` page flips and `BSPE/Present/` queue dequeuing.
* **Allowed Includes:** `bspe.h`, `swapchain.h`.
* **Forbidden Includes:** Shell applications, BOGE rendering primitives, raw video I/O ports.
