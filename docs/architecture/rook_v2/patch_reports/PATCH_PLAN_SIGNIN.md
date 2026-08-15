# ♜ ATOMS OS — Architecture Patch Plan
## Task 2: Sign-In Page Stride Invariant & Coordinate Alignment Repair

**Plan ID:** `PATCH-PLAN-SIGNIN-STRIDE-001`  
**Phase:** `TASK 2 — ARCHITECT TEAM (PLANNING PHASE — NO CODE EDIT)`  
**Target Hardware:** Intel Core i3 Haswell LGA1150 / H81 Motherboard / 8GB DDR3 RAM / 2022 UEFI Mode  

---

### 1. Architectural Scope & Objectives

Fix the horizontal 4X repeating strip artifact and top-edge squish on the Sign-In Page by restoring the strict RAM Surface Invariant across all sign-in render passes:
1. **Direct Stride Pixel Invariant:** Enforce `stride_pixels = width` ($1920$) across `premium_signin_renderer.h` and `user_profile_service.c`, eliminating all erroneous `stride / 4` divisions.
2. **Accurate Font Byte Pitch:** Enforce `target.pitch = width * 4` ($7680$ bytes) in `premium_center_text()` for `BOFont` rendering.
3. **Centered Geometric Layout:** Ensure Lock Mark, Circular User Avatar ($48\text{px}$ radius), `"Welcome, Admin"` title, Frosted Glass Password Box ($340 \times 52\text{px}$), and `"Sign In"` button ($180 \times 44\text{px}$) are centered symmetrically at $cx, cy$.

---

### 2. Files to be Modified

| File | Purpose of Modification |
| :--- | :--- |
| `kernel/shell/rook/pages/premium_signin_renderer.h` | Enforce `stride_pixels = width`, `stride_bytes = width * 4`, fix text pitch & box coordinates |
| `kernel/services/user_profile/user_profile_service.c` | Enforce `stride_pixels = fb_w` in `user_profile_service_render_avatar` |

---

### 3. Expected Results & Telemetry Verification

* **Visual Alignment:** Exactly 1 single centered Sign-In card with dark blurred mountain wallpaper, crystal-clear circular avatar, centered `"Welcome, Admin"` text, and glowing password entry box.
* **Repetition Count:** 0 repeating horizontal artifacts (0% duplication).
* **Frame Render Cost:** $< 0.05\text{ms}$ on Intel Haswell Core i3.
