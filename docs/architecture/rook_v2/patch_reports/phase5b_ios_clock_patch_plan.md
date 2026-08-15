# ♜ ATOMS OS — Architecture Patch Plan
## Phase 5B: Apple iOS 17 SF Pro Display Heavy Bold Lock Screen Typography & Dual-Layer Compositor

**Plan ID:** `PLAN-ROOK-V2-PHASE5B-CLOCK`  
**Status:** `READY FOR EXECUTION`  
**Author:** Antigravity Architect Team  

---

### 1. Forensic Target & Scope
* **Target File:** [`kernel/shell/rook/pages/page_login.c`](file:///D:/Signatures_OS/kernel/shell/rook/pages/page_login.c)
* **Target Functions:**
  - `draw_large_time()` $\rightarrow$ Upgrade to Dual-Layer 4x MSAA SF Pro Display Heavy Bold Compositor.
  - `get_subpixel_alpha()` $\rightarrow$ Implement 16-sample 4x4 subpixel analytic curve & rounded-rectangle coverage engine for digits $0 \dots 9$ and colon `$:$`.
  - `page_login_on_render()` $\rightarrow$ Implement vertical optical hierarchy (Lock Icon at $cy - 200$, Date at $cy - 152$, Clock at $cy - 110$). Add Gregorian Day-of-Week generator from RTC.

---

### 2. Files Permitted to Modify
1. [`kernel/shell/rook/pages/page_login.c`](file:///D:/Signatures_OS/kernel/shell/rook/pages/page_login.c)

---

### 3. Verification Protocol
1. Zero errors on `build.ps1`.
2. QEMU pre-flight validation via `tools/capture_qemu_screenshot.py`.
3. Physical Haswell H81 hardware test over PXE network boot.
