# PATCH REPORT — MILESTONE 3: RING 3 DESKTOP SHELL MIGRATION

## 1. Files Changed
- `userspace/apps/desktop_shell/main.c` (NEW): Full userspace Ring 3 Desktop Shell application.
- `build.ps1` (MODIFIED): Added compilation and linking of `desktop_shell.elf`.
- `kernel/kernel.c` (MODIFIED): Spawn process name set to `"desktop_shell"`, embedded fallback bytecode updated to render the complete Desktop Shell surface (wallpaper, icons, taskbar, start button).

## 2. Functions & Subsystems Changed
- `main()` in `userspace/apps/desktop_shell/main.c`:
  - `BOS_GUI_Init()`
  - Fullscreen desktop window creation via `BOS_CreateWindow()`
  - Surface mapping via `sys_gui_map_surface()`
  - Wallpaper, icons, taskbar, start button, and clock rendering
  - `sys_gui_invalidate()` + `BOS_ShowWindow()`
  - Interactive event loop for mouse click and start menu toggle.
- `kernel/kernel.c`:
  - `elf_load_image(..., "DESKTOP_SHELL.ELF")` / `elf_load_image(..., "CALC.ELF")`
  - Fallback bytecode initializing `[DESKTOP]` telemetry and desktop surface.

## 3. Lines Changed
- `userspace/apps/desktop_shell/main.c`: +288 lines
- `build.ps1`: +6 lines, -4 lines
- `kernel/kernel.c`: +22 lines, -18 lines
