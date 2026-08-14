# PATCH_PLAN.md — 4X Stride Division Correction Architecture Plan

## Executive Summary
This document specifies the exact plan to fix the 4X stride division in `page_login.c` to render the Lock Screen Clock and icons centered properly on screen.

---

## 1. What to Modify

### Modification A: Correct stride_pixels in page_login.c
- **File**: [page_login.c](file:///d:/Signatures_OS/kernel/shell/rook/pages/page_login.c)
- **Plan**:
  1. Change `uint32_t stride_pixels = stride / 4;` to:
     `uint32_t stride_pixels = (stride >= width * 4) ? (stride / 4) : ((stride > 0) ? stride : width);`
  2. Ensures `stride_pixels` is $1920$ (NOT $480$).
  3. Lock Screen Clock (`11:51`) and icons render centered at `(cx, cy - 100)` in full size on screen.

---

## 2. Expected Result
- **Visual Presentation**:
  - Boot Splash finishes ➔ Chevron logo & spinner vanish.
  - Lock Screen Clock (`11:51`) and icons render **100% PERFECTLY CENTERED IN FULL SIZE** on the display.
  - Zero 4X top-strip repeating artifacts.

---

## 3. Rollback Plan
- Revert stride calculation if any layout mismatch occurs.

---
*Plan created by ATOMS OS Architect Team under Protocol V1 (NO CODE).*
