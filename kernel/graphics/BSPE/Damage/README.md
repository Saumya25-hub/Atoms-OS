# BSPE Damage Tracker Subsystem (`Damage/`)

> **Module:** BOS Surface Presentation Engine — Dual-Page Damage Tracker  
> **Status:** Step 2 Empty Production Module (Zero Logic)  

---

## 1. Responsibility
The `Damage/` subsystem calculates and enforces dual-page VRAM damage union math: $\text{EffectiveDamage} = \text{Damage}(N) \cup \text{Damage}(N-1)$. This ensures that partial VRAM copying across double-buffered pages never leaves trailing cursor artifacts or ghost window frames.

## 2. Dependencies
* **Upstream:** Receives dirty rectangle lists from `BSPE/Present/` (Present Queue).
* **Downstream:** Feeds effective clipping spans to `BSPE/DisplayHAL/` for partial MMIO bus copying.
* **Allowed Includes:** `bspe.h`, `display_hal.h`.
* **Forbidden Includes:** BOGE widget rendering loops, window manager hit-testing, video driver internals.
