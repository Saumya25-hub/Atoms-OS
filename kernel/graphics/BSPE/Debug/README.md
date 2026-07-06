# BSPE Real-Time Debug & Telemetry HUD (`Debug/`)

> **Module:** BOS Surface Presentation Engine — Telemetry HUD  
> **Status:** Step 2 Empty Production Module (Zero Logic)  

---

## 1. Responsibility
The `Debug/` subsystem provides non-destructive, real-time graphical overlays and kernel diagnostic logging for presentation performance. It tracks frame rates (FPS), frame presentation time (ms), dirty rectangle counts, MMIO bus bandwidth consumption (KB/frame), and swapchain buffer states.

## 2. Dependencies
* **Upstream:** Receives telemetry statistics from `BSPE/Present/`, `BSPE/Swapchain/`, and `BSPE/Damage/`.
* **Downstream:** Outputs diagnostic metrics to kernel serial log (`bootlog.txt`) and renders optional HUD overlays onto staging frames.
* **Allowed Includes:** `bspe.h`, `present_queue.h`, `swapchain.h`, `damage_tracker.h`.
* **Forbidden Includes:** Application business logic, window manager layout headers.
