# FORENSIC REPORT — ATOMS OS Desktop Icon System Redesign

## 1. Executive Summary
While the ATOMS OS taskbar was successfully upgraded to official high-definition PNG icons, the desktop shell displayed 4 colored rectangle placeholders with 1px white borders:
- **Computer** (Solid Blue `#2563EB`)
- **Files** (Solid Green `#10B981`)
- **Terminal** (Solid Cyan `#38BDF8`)
- **Settings** (Solid Orange `#F59E0B`)

Forensic source tracing isolated the root cause to `userspace/apps/desktop_shell/main.c` (PID=200, CPL=3), which creates a full-screen borderless window and directly renders hardcoded `draw_border_rect()` colored squares onto its private mapped framebuffer surface instead of utilizing the official compile-time RGBA icon bitmaps and bilinear downscaling pipeline.

---

## 2. Forensic Evidence & Root Cause Analysis

### Evidence A: Hardcoded Colored Rectangles in `userspace/apps/desktop_shell/main.c`
In `userspace/apps/desktop_shell/main.c` lines 148–185:
```c
static DesktopIcon g_icons[] = {
    { "Computer", 40, 40,  80, 80, 0xFF2563EB, false },
    { "Files",    40, 140, 80, 80, 0xFF10B981, false },
    { "Terminal", 40, 240, 80, 80, 0xFF38BDF8, false },
    { "Settings", 40, 340, 80, 80, 0xFFF59E0B, false }
};

// Line 181:
draw_border_rect(surface, width, ix + 16, iy + 6, 48, 48, g_icons[i].icon_color, 0xFFFFFFFF);
```
- `draw_border_rect` draws a solid 48×48 box filled with `icon_color` and framed with a `0xFFFFFFFF` white border.
- The wallpaper is drawn via Syscall 24, but the 4 icon positions are overwritten with primitive colored boxes.

### Evidence B: Missing `Computer / This PC` Official Master PNG Asset
- `assets/icons/apps/` has `explorer.png`, `terminal.png`, `settings.png`, but currently lacks a dedicated, standalone **`computer.png`** (Workstation / This PC) master 256×256 asset.

---

## 3. Files Involved
1. `tools/icon_pipeline.py`: Requires addition of `render_computer()` master 256×256 generator and export of `g_atoms_ico_computer_48`.
2. `kernel/ui/icon_engine/include/atoms_icon_data.h`: Master compile-time RGBA array export.
3. `kernel/ui/icon_engine/include/icon_engine.h`: Add `ICON_ID_COMPUTER` / `ICON_ID_THIS_PC`.
4. `kernel/ui/icon_engine/src/icon_engine.c`: Register `ICON_ID_COMPUTER`.
5. `userspace/apps/desktop_shell/main.c`: Replace `draw_border_rect` with embedded 48×48 alpha-blended bilinear icon resampling and transparent selection halo.
6. `kernel/shell/desktop_shell/desktop_shell.c`: Synchronize kernel-level desktop icon handlers.

---

## 4. Risk Analysis
- **Zero Kernel Regressions**: The changes in `userspace/apps/desktop_shell/main.c` are isolated to userspace rendering.
- **Zero Allocations**: Resampling and alpha-blending onto the userspace mapped surface will use fixed-point arithmetic on stack memory with zero dynamic allocations.
