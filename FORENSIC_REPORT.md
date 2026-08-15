# 🔬 FORENSIC INVESTIGATION REPORT: LOCK SCREEN VS LOGIN SCREEN POWER CONTROLS ISOLATION
**Subsystem:** ATOMS OS Rook Shell (`kernel/shell/rook/pages/page_login.c`)  
**Investigating Agent:** Antigravity / ARYA Core Forensic  
**Date:** 2026-08-16  
**Status:** TASK 1 COMPLETE (Forensic Phase — NO CODE)

---

## 1. Executive Summary & Root Cause
* **Root Cause:** In `kernel/shell/rook/pages/page_login.c`, the bottom-right corner power controls (Restart `↻` & Shutdown `⏻`) were rendered inside the `s_lock_alpha > 0` (Lock Screen) block instead of the `s_signin_alpha > 0` (Sign-In / Login Screen) block.
* **Impact:** Power controls were visible on the lock screen before swiping/clicking up to sign in, violating the lock screen UI design specification.

---

## 2. Real OS Standard (Windows 11 / macOS / iOS)
* **Lock Screen:** Minimalist ambient display showing only Clock, Date, Lock Status, and bottom status capsules (Ethernet/Chat). Power controls are completely hidden.
* **Login / Sign-In Screen:** User Avatar, Password Box, Sign-In Submit button, and bottom-right corner power controls (Restart & Shutdown).

---

## 3. Files Involved
* `kernel/shell/rook/pages/page_login.c`: Move corner power controls rendering and hit-testing exclusively to the `s_signin_alpha > 0` / `LOGIN_STATE_SIGN_IN` block.
