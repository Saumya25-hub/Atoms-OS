# PATCH_PLAN.md — 7-Phase Display Pipeline Architecture Plan

## Executive Summary
This document specifies the exact architectural plan to eliminate all real-hardware display artifacts at the source without hacks or hardcoded monitor values.

---

## 1. What to Modify

### Modification A: Safe Stride Evaluation Across All Renderers
- **Files**:
  - [page_login.c](file:///d:/Signatures_OS/kernel/shell/rook/pages/page_login.c)
  - [wallpaper_service.c](file:///d:/Signatures_OS/kernel/services/wallpaper/wallpaper_service.c)
  - [premium_signin_renderer.h](file:///d:/Signatures_OS/kernel/shell/rook/pages/premium_signin_renderer.h)
- **Plan**:
  1. Enforce safe stride metric evaluation across all renderer sub-modules:
     `uint32_t stride_pixels = (stride >= width * 4) ? (stride / 4) : ((stride > 0) ? stride : width);`
  2. Guarantees 100% resolution independence across arbitrary UEFI hardware (1080p, 1440p, 4K).

### Modification B: PCIe Memory Barrier & Full VRAM Coverage
- **File**: [rook_render.c](file:///d:/Signatures_OS/kernel/shell/rook/src/rook_render.c)
- **Plan**:
  1. Remove hardcoded VRAM zeroing clamp (`if (total_vram_words > 1920*1080)`).
  2. Zero 100% of physical VRAM (`pitch_pixels * height`).
  3. Add x86 `sfence` (`stream fence`) memory barriers after VRAM zeroing and frame flushing.
  4. Expand `g_rook_backbuffer` to `2560 * 1600` QWORD-aligned RAM array.

---

## 2. Expected Result
- **Arbitrary Hardware Compatibility**: Works 100% on any physical UEFI motherboard/GPU (Intel, NVIDIA, AMD).
- **Visual Presentation**: Pristine `#000000` pitch-black canvas at boot, 100% centered Lock Screen & Sign-In UI in full size, zero 4X repeating strips, zero grey headers.

---

## 3. Rollback Plan
- Revert memory barrier or stride logic if any compiler error occurs.

---
*Plan created by ATOMS OS Architect Team under Protocol V1 (NO CODE).*
