# TASKBAR STABLE IMPLEMENTATION

**Target Physical Hardware Profile**:
- Motherboard: H81 Motherboard (Haswell LGA1150 Chipset)
- BIOS Firmware: 2022 Updated BIOS (Native UEFI Mode)
- CPU: Intel Core i3 4th Gen (Haswell x86_64)
- RAM: 8 GB RAM

---

## 1. Single Authoritative Geometry Authority (`Taskbar_Layout`)

All taskbar metrics are derived deterministically in `task_panel_compute_layout()` from active framebuffer dimensions:

- **Capsule Dimensions**:
  - `height`: 52 logical pixels
  - `bottom_margin`: 12 logical pixels
  - `corner_radius`: 16 pixels
  - `x`: Centered horizontally via `(screen_w - total_w) / 2`
  - `y`: `screen_h - 52 - 12`
- **Left Zone (Start Pill)**:
  - `width`: 78 pixels, `height`: 36 pixels, `corner_radius`: 12 pixels
  - Contains 20x20 ATOMS emblem and "Start" label.
- **Center Zone (Core App Slots)**:
  - 10 Pinned Applications with fixed `38x38` cells and `6px` spacing.
  - 32x32 transparent icons centered inside slot.
  - 18px cyan running indicator bar for focused window; 5px dot for background window.
- **Right Zone (System Tray & RTC Clock)**:
  - 16x16 Wi-Fi Status Icon (`ICON_SYS_WIFI_CONN`).
  - 16x16 Volume Status Icon (`ICON_SYS_VOL_NORM`).
  - 2-Line RTC Clock (`HH:MM AM/PM` and `DD Mon`).

---

## 2. Deterministic Rendering Pipeline

Rendering strictly follows the top-to-bottom paint sequence:
1. **Wallpaper & Windows Composition**
2. **Capsule Background & 1px Highlight Rim** (Euclidean distance rounded pill math)
3. **Start Pill Container & Emblem/Text**
4. **Application Icons & Running Status Bars/Dots**
5. **System Tray Icons & RTC Clock**
6. **Full-Frame Synchronized Presentation to Double-Buffered VRAM**

---

## 3. Input & Invalidation Model

- Mouse move events perform O(1) hit testing against pre-computed `s_layout` bounding boxes.
- Invalidation (`BWE_InvalidateWindow(g_task_panel_win_id)`) triggers **only** when `hover_index` changes.
- Frame-to-frame geometry remains 100% invariant during mouse motion.
