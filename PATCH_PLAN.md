# 📐 ARCHITECTURE PATCH PLAN: LOCK SCREEN VS LOGIN SCREEN POWER CONTROLS ISOLATION
**Subsystem:** ATOMS OS Rook Shell (`kernel/shell/rook/pages/page_login.c`)  
**Lead Architect:** Antigravity / ARYA Core Architect  
**Date:** 2026-08-16  
**Status:** TASK 2 COMPLETE (Architecture Phase — NO CODE MODIFIED)

---

## 1. Objectives & Scope
- Remove the bottom-right corner power controls from the `s_lock_alpha > 0` lock screen render path.
- Add the bottom-right corner power controls to the `s_signin_alpha > 0` login screen render path, scaled smoothly by `s_signin_alpha`.
- Restrict power control click detection exclusively to the `s_login_state == LOGIN_STATE_SIGN_IN || s_signin_alpha > 0` state.

---

## 2. Target Files for Modification
1. `kernel/shell/rook/pages/page_login.c`:
   - Move corner power controls render block to `s_signin_alpha > 0`.
   - Update `page_login_on_update()` mouse click hit-testing.

---

## 3. Expected Engineering Results
- **Lock Screen:** Zero power controls visible.
- **Login / Sign-In Screen:** Power controls appear smoothly with the password and avatar UI.

---

## 4. Rollback Plan
Revert changes to Git commit `4a002e2`.
