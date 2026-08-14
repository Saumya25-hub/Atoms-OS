# PATCH_PLAN.md — Interactive Supervisor Loop for ROOK_PAGE_LOGIN

## Executive Summary
This document specifies the exact plan to implement `rook_login_spin()` in `rook_core.c` and invoke it from `kernel.c` to drive the interactive Stage 2 Login Screen.

---

## 1. What to Modify

### Modification A: Interactive Login Loop in rook_core.c
- **File**: [rook_core.c](file:///d:/Signatures_OS/kernel/shell/rook/src/rook_core.c)
- **Plan**:
  1. Implement `rook_login_spin(void)` supervisor loop.
  2. While `g_current_page->id == ROOK_PAGE_LOGIN`, continuously invoke `rook_update(16)` and `rook_render()` with Hardware TSC 60.00 FPS pacing.
  3. When authentication succeeds and page transitions to `ROOK_PAGE_DESKTOP`, exit `rook_login_spin()` smoothly.

### Modification B: Invoke rook_login_spin in kernel.c
- **File**: [kernel.c](file:///d:/Signatures_OS/kernel/kernel.c)
- **Plan**:
  1. Call `rook_login_spin()` immediately after `rook_goto(ROOK_PAGE_LOGIN)`.

---

## 2. Expected Result
- **Boot Handoff**: 6.0s Boot Splash completes ➔ Windows 11 Lock Screen appears smoothly.
- **Interactivity**: Keyboard typing (`admin123`) and mouse clicks execute in real-time with 60 FPS hardware TSC pacing.
- **Desktop Handoff**: Successful authentication transitions into the Ring 3 Desktop Shell.

---

## 3. Rollback Plan
- Revert `rook_login_spin` if any infinite loop issue occurs.

---
*Plan created by ATOMS OS Architect Team under Protocol V1 (NO CODE).*
