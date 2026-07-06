# BSPE Hardware Cursor Plane Subsystem (`Cursor/`)

> **Module:** BOS Surface Presentation Engine — Hardware Cursor Plane  
> **Status:** Step 2 Empty Production Module (Zero Logic)  

---

## 1. Responsibility
The `Cursor/` subsystem controls the asynchronous hardware sprite overlay plane (Bochs VGA / VBE registers `0x03D4`/`0x03D5`). It decouples mouse rendering from the compositor, achieving zero-damage, zero-cost mouse movement. It also provides an asynchronous software fallback for legacy VESA hardware.

## 2. Dependencies
* **Upstream:** Receives coordinate updates directly from PS/2 and USB mouse drivers (`mouse_engine.c`).
* **Downstream:** Communicates with `BSPE/DisplayHAL/` to execute hardware register writes or fallback blits.
* **Allowed Includes:** `bspe.h`, `display_hal.h`.
* **Forbidden Includes:** `bwe_compositor.h`, window manager Z-stacks, BOGE widget rendering loops.
