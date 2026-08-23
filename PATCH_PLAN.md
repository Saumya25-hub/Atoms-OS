# PATCH PLAN — ATOMS OS Desktop Icon System Redesign

## 1. Goal
Upgrade the 4 desktop icons (`Computer`, `Files`, `Terminal`, `Settings`) from colored box placeholders to the official ATOMS OS premium PNG visual assets, featuring sub-pixel alpha transparency, glass selection halos, and zero heap allocations.

---

## 2. Architectural Design & Pipeline

### Phase A: Master Asset Creation (`assets/icons/apps/computer.png`)
In `tools/icon_pipeline.py`:
- Implement `render_computer()`:
  - High-end dark graphite + metallic cyan/blue workstation tower + widescreen LED monitor with subtle 3D depth and clean modern silhouette.
  - Transparent background, 4x supersampled 256×256 master PNG.
- Export `g_atoms_ico_computer_48` to `atoms_icon_data.h`.

### Phase B: Compile-Time Embedded RGBA Assets for Userspace & Kernel
- Export clean 48×48 RGBA byte arrays into `userspace/apps/desktop_shell/desktop_icon_data.h` and `kernel/ui/icon_engine/include/atoms_icon_data.h`.
- Add `ICON_ID_COMPUTER` to `IconId` enum in `icon_engine.h` and register in `icon_engine.c`.

### Phase C: Userspace Desktop Shell Rendering (`userspace/apps/desktop_shell/main.c`)
- Remove `draw_border_rect(surface, width, ix + 16, iy + 6, 48, 48, g_icons[i].icon_color, 0xFFFFFFFF);`.
- Implement `draw_desktop_icon_rgba(surface, width, height, ix + 20, iy + 4, icon_sz, icon_sz, icon_data, state)`:
  - 16.16 fixed-point bilinear resampling down to 40×40 px.
  - Sub-pixel alpha blending into the mapped desktop surface.
  - Normal, Hover (+24 brightness highlight), and Selected (translucent glass tile: `0x3338BDF8` background with `0x8038BDF8` 1px border) states.
- Clean centered typography for icon labels with drop shadow for optimal contrast against any wallpaper.

### Phase D: Kernel Desktop Shell Alignment (`kernel/shell/desktop_shell/desktop_shell.c`)
- Update `icon_render_callback` and `BOS_CreateDesktopIcon` in `surface.c` to map `Computer` to `ICON_ID_COMPUTER` instead of `ICON_ID_EXPLORER`.

---

## 3. Rollback Plan
If any visual artifact occurs, backup copies of `userspace/apps/desktop_shell/main.c` can be restored instantly.
