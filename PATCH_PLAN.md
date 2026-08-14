# PATCH_PLAN.md — PCIe Memory Barrier & LAN Debug Architecture Plan

## Executive Summary
This document specifies the plan to add x86 `sfence` PCIe memory barriers, force full-frame invalidation during boot splash, and activate UDP LAN debug telemetry.

---

## 1. What to Modify

### Modification A: x86 PCIe Memory Barrier in rook_render.c
- **File**: [rook_render.c](file:///d:/Signatures_OS/kernel/shell/rook/src/rook_render.c)
- **Plan**:
  1. Add `__asm__ volatile("sfence" ::: "memory");` after VRAM zeroing in `rook_init_renderer`.
  2. Add `__asm__ volatile("sfence" ::: "memory");` at the end of `rook_render_flush()`.
  3. Guarantees 100% of CPU write-combining stores are committed across the PCIe bus into GPU physical VRAM.

### Modification B: Full Frame Invalidation in page_boot.c
- **File**: [page_boot.c](file:///d:/Signatures_OS/kernel/shell/rook/pages/page_boot.c)
- **Plan**:
  1. Force `s_first_frame = true` on `boot_page_on_enter`.
  2. Always execute full-frame copy and `rook_invalidate_full()` on every boot splash frame to guarantee pristine `#000000` black canvas across 100% of the display.

### Modification C: UDP LAN Debug Activation
- **File**: [kernel.c](file:///d:/Signatures_OS/kernel/kernel.c)
- **Plan**:
  1. Ensure UDP LAN Debug logger is active on port `9999` for real-time telemetry streaming to host.

---

## 2. Expected Result
- **Visual Presentation**:
  - Boot Splash renders **100% PURE PRISTINE `#000000` BLACK CANVAS ACROSS THE ENTIRE PHYSICAL DISPLAY** (Zero grey headers, zero `====` lines, zero text artifacts).
  - 6.0s AME Spinner rotates liquid-smooth at 60 FPS.
  - Seamless transition to 100% centered Windows 11 Lock Screen.
- **Telemetry**: Real-time UDP LAN Debug logs stream live to port `9999`.

---

## 3. Rollback Plan
- Revert memory barrier instructions if any compiler inline assembly error occurs.

---
*Plan created by ATOMS OS Architect Team under Protocol V1 (NO CODE).*
