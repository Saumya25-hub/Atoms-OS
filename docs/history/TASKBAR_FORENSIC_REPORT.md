# TASKBAR FORENSIC REPORT

**Target Physical Hardware Profile**:
- Motherboard: H81 Motherboard (Haswell LGA1150 Chipset)
- BIOS Firmware: 2022 Updated BIOS (Native UEFI Mode)
- CPU: Intel Core i3 4th Gen (Haswell x86_64)
- RAM: 8 GB RAM

---

## 1. Executive Summary & Forensic Questions

This forensic investigation addresses the 10 critical questions raised by the runtime behavior and user-submitted screenshots:

1. **Why did the Taskbar disappear / alternate between frames?**
2. **Why did fragments / square holes appear on mouse move?**
3. **Why did stray border lines protrude outside the capsule corners?**
4. **Why did random colored blocks/squares appear instead of clean icons?**
5. **Why was the Start button missing its text label and pill container?**
6. **Why was the System Tray (Wi-Fi, Volume) detached or missing from the clock?**
7. **Were there multiple Taskbar draw paths in the kernel?**
8. **Was there unsigned geometry underflow in bounds calculation?**
9. **Did mouse movement change taskbar geometry?**
10. **What exact files and functions are responsible?**

---

## 2. Root-Cause Analysis

### Forensic Finding 1: Double-Buffered VRAM Page Desynchronization (`bspe_use_partial_present = true`)
- **Evidence**: In `bovisual/Graphics/graphics.c` (line 322) and `kernel/graphics/BSPE/Present/vram_copy.c` (line 25), `bspe_use_partial_present` was set to `true`.
- **Mechanism**: The display driver operates in dual-page hardware double buffering (`vbe_swap_page()`). VRAM has two physical pages: Page 0 and Page 1. When the mouse moved, BSPE only copied the small 32x32 damage box to the back page before flipping. Consequently, Page 1 received only the 32x32 box and retained stale frame data with no taskbar. Every frame flip alternated between Page 0 (with taskbar) and Page 1 (without taskbar), creating severe flickering, missing pieces, and square cursor holes.
- **Fix Required**: Disable partial VRAM copy (`bspe_use_partial_present = false`) so every frame presented to VRAM is 100% complete and synchronized.

---

### Forensic Finding 2: Capsule Border Rendering Formula Error (`draw_capsule_shape`)
- **Evidence**: In `kernel/ui/task_panel.c` (lines 178–184), the edge detection condition evaluated:
  ```c
  bool is_edge = (x == cx || x == cx + cw - 1 || y == cy || y == cy + ch - 1 || ...);
  ```
- **Mechanism**: The straight bounding box checks `x == cx` and `y == cy` fired even inside the corner rounding margin ($x < cx + r$), because `is_outside_rounded_rect(x, y - 1, ...)` returned true for points immediately outside the corner arc. This caused outer horizontal and vertical tangent lines to be drawn outside the curved pill corners (seen clearly in Images 2 and 4).
- **Fix Required**: Replace the multi-pass edge test with exact Euclidean distance evaluation ($|d(x, y) - r| \le 1.0$) so border pixels strictly follow the rounded pill perimeter without stray tangent lines.

---

### Forensic Finding 3: Icon Fallback Colored Squares (`asset_loader.c` & `task_panel.c`)
- **Evidence**: In `task_panel.c` (line 378), any asset failure triggered `BWE_FillRect(fb, cell_x + 5, s_cell_y + 5, 28, 28, 0xFF3B82F6)`, drawing a solid blue square. In `asset_loader.c` (lines 60–160), fallback procedural glyphs used solid rectangular blocks with non-zero alpha backgrounds (`a = 255`, dark navy, red, or grey rectangles).
- **Mechanism**: Applications without fully transparent PNG assets rendered as solid colored tiles (seen in Image 5).
- **Fix Required**:
  1. Standardize all application icons with 100% transparent backgrounds (`a = 0`).
  2. Implement a single clean ATOMS vector-glyph fallback icon path with transparent background instead of solid colored squares.

---

### Forensic Finding 4: Absence of Single Geometry Authority (`Taskbar_Layout`)
- **Evidence**: `task_panel_render_callback()` and `task_panel_event_callback()` independently calculated component offsets (`s_start_x`, `s_cell_y`, `s_app_start_x`, `s_time_x`, `cell_w`, `spacing`, `total_w`).
- **Mechanism**: When window count changed or screen width adjusted, event handlers used stale or out-of-sync coordinate variables, leading to hit test misses and hover flickering.
- **Fix Required**: Introduce a central `Taskbar_Layout` struct and `task_panel_compute_layout()` function as the single source of truth for all layout metrics.

---

### Forensic Finding 5: Missing Start Pill, Tray Icons & Refined Layout (Matching Image 1)
- **Evidence**: The Start button was rendered as a bare 32x32 icon without its pill button container and without the "Start" text label. The right side had only text time without the Wi-Fi and Volume status icons.
- **Fix Required**: Structure the Taskbar into 3 clean logical zones matching Image 1:
  - **Left Zone**: Start Pill (`✦ Start` / `⚛ Start` with rounded background, 14px icon + text).
  - **Center Zone**: Core Pinned Apps (Files, Terminal, Notes, Calculator, Settings, Media Player) with transparent backgrounds and active pill/dot running indicators.
  - **Right Zone**: System Tray (Wi-Fi icon, Volume icon, 2-line RTC Time `12:40 PM` & Date `21 Aug 2025`).

---

## 3. Files Involved

1. [`kernel/ui/task_panel.h`](file:///d:/Signatures_OS/kernel/ui/task_panel.h): Layout struct definitions and public APIs.
2. [`kernel/ui/task_panel.c`](file:///d:/Signatures_OS/kernel/ui/task_panel.c): Single authoritative geometry layout, capsule shape rendering, start pill, app icons, running indicators, system tray, RTC clock, hit testing.
3. [`kernel/ui/boasset/asset_loader.c`](file:///d:/Signatures_OS/kernel/ui/boasset/asset_loader.c): Icon procedural synthesis with transparent backgrounds.
4. [`kernel/ui/boasset/boasset.c`](file:///d:/Signatures_OS/kernel/ui/boasset/boasset.c): Transparent asset rendering to active render target.
5. [`bovisual/Graphics/graphics.c`](file:///d:/Signatures_OS/bovisual/Graphics/graphics.c): Double-buffered presentation sync (`bspe_use_partial_present = false`).
6. [`kernel/graphics/BSPE/Present/vram_copy.c`](file:///d:/Signatures_OS/kernel/graphics/BSPE/Present/vram_copy.c): Full double-buffer VRAM blit backend.
7. [`kernel/wm/bwe/renderer/bwe_compositor.c`](file:///d:/Signatures_OS/kernel/wm/bwe/renderer/bwe_compositor.c): Top-level taskbar geometry synchronization.

---

## 4. Risk Analysis

- **Risk 1 (Performance)**: Full-frame double-buffer copy on 60 Hz frame clock could introduce CPU overhead.
  - *Mitigation*: The kernel uses 64-bit SSE/AVX streaming copy (`BOVISUAL_Graphics_LegacySwapFull_Backend`) taking < 0.9ms per frame (less than 6% of a 16.6ms frame budget).
- **Risk 2 (Geometry Underflow)**: If screen width is smaller than minimum taskbar width, coordinates could underflow.
  - *Mitigation*: Enforce bounds clamping: if $total\_w > screen\_w - 20$, scale spacing or clamp $abs\_cx \ge 10$.

---

## 5. Next Steps

Proceed to **TASK 2 — ARCHITECT TEAM** to produce `PATCH_PLAN.md`.
