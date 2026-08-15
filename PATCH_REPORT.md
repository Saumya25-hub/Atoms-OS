# 🛠️ PATCH REPORT: LOCK SCREEN VS LOGIN SCREEN POWER CONTROLS ISOLATION
**Subsystem:** ATOMS OS Rook Shell (`kernel/shell/rook/pages/page_login.c`)  
**Patch Engineer:** Antigravity / ARYA Core Patch Team  
**Date:** 2026-08-16  
**Status:** TASK 3 COMPLETE (Patch Phase)

---

## 1. Files & Functions Changed

### `kernel/shell/rook/pages/page_login.c`
* **Functions:** `page_login_on_update()` & `page_login_on_render()`
* **Changes:**
  - Removed Restart (`↻`) and Shutdown (`⏻`) button rendering from the `s_lock_alpha > 0` lock screen path.
  - Placed power controls inside the `s_signin_alpha > 0` path so they only appear on the active Sign-In / Login page.
  - Restricted click hit-testing for power controls to `s_login_state == LOGIN_STATE_SIGN_IN || s_signin_alpha > 0`.
