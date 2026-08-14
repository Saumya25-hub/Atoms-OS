# PATCH_PLAN.md — Stage 2 Login Page Input Queue Drain Architecture Plan

## Executive Summary
This document specifies the exact plan to implement keyboard event queue draining in `page_login_on_update` to guarantee 100% zero-latency typing and authentication.

---

## 1. What to Modify

### Modification A: Scancode Queue Draining in page_login.c
- **File**: [page_login.c](file:///d:/Signatures_OS/kernel/shell/rook/pages/page_login.c)
- **Plan**:
  1. Wrap keyboard processing in `while (keyboard_poll_event(&key_evt))` inside `page_login_on_update`.
  2. Process all pending keystrokes per frame for instant password character entry (`admin123`).
  3. Maintain seamless transition to `ROOK_PAGE_DESKTOP` upon authentication success (`LOGIN_STATE_AUTH_SUCCESS`).

---

## 2. Expected Result
- **Typing Responsiveness**: Instant character entry with zero key drops.
- **Authentication Handoff**: Smooth 250ms fade-out transition into the Ring 3 Desktop Shell (`ROOK_PAGE_DESKTOP`).

---

## 3. Rollback Plan
- Revert loop to single `keyboard_poll_event` call if any unexpected scancode repetition occurs.

---
*Plan created by ATOMS OS Architect Team under Protocol V1 (NO CODE).*
