# BOS Surface Presentation Engine (BSPE) — Pure Presentation Engine

> **Module:** Core Presentation, Swapchain & VSync Engine  
> **Status:** Phase 0 Architecture Frozen  
> **Target Path:** `kernel/graphics/BSPE/`  

---

## 1. Engine Responsibility

**BSPE** is strictly responsible for receiving completed staging frames from BOGE V2 and presenting them to the physical display controller with zero visual tearing, minimal latency, and maximum bus efficiency:
1. **Present Queue (`Present/`):** Dequeues completed staging frame handles submitted by BOGE V2.
2. **Frame Pacing & VSync (`FramePacer/`):** Aligns VRAM memory copying and page flipping with monitor Vertical Blanking Intervals (VBI).
3. **Dual-Page Damage Tracking (`Damage/`):** Computes $\text{EffectiveDamage} = \text{Damage}(N) \cup \text{Damage}(N-1)$, eliminating trailing cursor artifacts without forcing 3.14 MB full-screen copies (`SwapFull`).
4. **Swapchain Management (`Swapchain/`):** Manages double and triple buffer rotation between front, back, and staging buffers.
5. **Hardware Cursor Plane (`Cursor/`):** Updates Bochs VGA / VBE hardware cursor registers asynchronously, achieving zero-damage, zero-cost mouse movement.
6. **Display HAL & Video Drivers (`DisplayHAL/`, `Drivers/`):** Decouples presentation from physical Bochs VBE, VGA, VESA, VirtIO, and future GPU hardware drivers.

---

## 2. Subdirectory Architecture

- `Present/` — Present queue ring buffer and presentation loops.
- `Swapchain/` — Double / Triple buffer swapchain state managers.
- `Damage/` — Dual-page VRAM damage history trackers (`g_page_damage[2]`).
- `FramePacer/` — VSync IRQ synchronization and timer pacing engines.
- `Cursor/` — Hardware cursor plane engine and asynchronous software sprite fallback.
- `DisplayHAL/` — Hardware Abstraction Layer interface tables (`BSPE_DisplayDriver`).
- `Drivers/` — Video driver backends (Bochs VBE I/O port writers, VESA BIOS, VirtIO GPU HAL).
- `Debug/` — Real-time telemetry HUD overlays (FPS, damage count, bandwidth metrics).
- `include/` — Internal and public BSPE header definitions (`bspe.h`).

---

## 3. Architectural Rule

> **"BSPE shall never inspect UI layout coordinates, never read or modify window backing bitmaps, and never execute line/shape drawing primitives. It operates exclusively on completed staging frames and dirty rectangles."**
