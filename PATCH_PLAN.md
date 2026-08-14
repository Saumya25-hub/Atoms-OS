# PATCH_PLAN.md — Permanent Full VRAM Zero Fill & 2560x1600 Double Buffer Plan

## Executive Summary
This document specifies the exact plan to eliminate the top blue banner artifact permanently by zeroing 100% of physical VRAM and expanding the double buffer to 2560x1600 resolution.

---

## 1. What to Modify

### Modification A: Full Physical VRAM 64-Bit Zero Fill in rook_render.c
- **File**: [rook_render.c](file:///d:/Signatures_OS/kernel/shell/rook/src/rook_render.c)
- **Plan**:
  1. Remove hardcoded `1920 * 1080` VRAM zeroing clamp.
  2. Compute total VRAM words as `pitch_pixels * height`.
  3. Execute 64-bit uint64_t zero fill across all VRAM words (`0x0000000000000000ULL`) during `rook_init_renderer`.
  4. Guarantees 100% instant wipe of UEFI BIOS blue console header memory on all GPUs (Intel / NVIDIA / AMD).

### Modification B: Expand g_rook_backbuffer to 2560x1600 Support
- **File**: [rook_render.c](file:///d:/Signatures_OS/kernel/shell/rook/src/rook_render.c)
- **Plan**:
  1. Expand static array size `g_rook_backbuffer[2560 * 1600]`.
  2. Enable double-buffering for all resolutions up to 2560x1600.

---

## 2. Expected Result
- **Visual Presentation**: 100% Pristine `#000000` Black Canvas across the ENTIRE physical monitor (Zero blue banners, zero line artifacts).
- **PCIe Efficiency**: 64-Bit Dual-Pixel presentation maintained for 100% butter-smooth motion.

---

## 3. Rollback Plan
- Revert array size and loop boundaries if any link memory allocation issue occurs.

---
*Plan created by ATOMS OS Architect Team under Protocol V1 (NO CODE).*
