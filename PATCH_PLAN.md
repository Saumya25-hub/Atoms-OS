# PATCH_PLAN.md — Secondary Renderer Stride Division Correction Plan

## Executive Summary
This document specifies the exact plan to fix the stride calculation in `wallpaper_service.c` and `premium_signin_renderer.h`, and add runtime telemetry logging in `page_login.c`.

---

## 1. What to Modify

### Modification A: Correct Stride in wallpaper_service.c
- **File**: [wallpaper_service.c](file:///d:/Signatures_OS/kernel/services/wallpaper/wallpaper_service.c)
- **Plan**: Change `uint32_t stride_pixels = fb_stride / 4;` to:
  `uint32_t stride_pixels = (fb_stride >= fb_width * 4) ? (fb_stride / 4) : ((fb_stride > 0) ? fb_stride : fb_width);`

### Modification B: Correct Stride in premium_signin_renderer.h
- **File**: [premium_signin_renderer.h](file:///d:/Signatures_OS/kernel/shell/rook/pages/premium_signin_renderer.h)
- **Plan**: Change `uint32_t stride_pixels = stride_bytes / 4u;` to:
  `uint32_t stride_pixels = (stride_bytes >= width * 4) ? (stride_bytes / 4) : ((stride_bytes > 0) ? stride_bytes : width);`

### Modification C: Runtime Telemetry in page_login.c
- **File**: [page_login.c](file:///d:/Signatures_OS/kernel/shell/rook/pages/page_login.c)
- **Plan**: Print live COM1/LAN telemetry in `page_login_on_render`:
  `[LOGIN RENDER METRICS] width=1920 height=1080 stride=1920 stride_pixels=1920`

---

## 2. Expected Result
- **Runtime Telemetry**: Streams exact runtime log `stride_pixels = 1920`.
- **Visual Presentation**: Lock Screen Clock and Sign-In UI render 100% centered in full size on a pristine dark canvas with zero 4X horizontal repeating artifacts.

---

## 3. Rollback Plan
- Revert stride calculations if any layout mismatch occurs.

---
*Plan created by ATOMS OS Architect Team under Protocol V1 (NO CODE).*
