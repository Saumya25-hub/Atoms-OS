# PATCH PLAN — MILESTONE 3: RING 3 DESKTOP SHELL MIGRATION

## 1. What to Modify
1. Create `userspace/apps/desktop_shell/main.c` implementing the full Ring 3 Desktop Shell application:
   - Fullscreen desktop surface creation.
   - Deep slate / mountain wallpaper rendering.
   - Desktop application icons: `[My Computer]`, `[Files]`, `[Terminal]`, `[Settings]` with title text and icons.
   - Bottom Taskbar (48px height) with `START` button and clock indicator.
   - Interactive event loop for mouse move/click detection, icon highlight, start menu toggle, and keyboard actions.
2. Update `build.ps1` to compile `userspace/apps/desktop_shell/main.c` into `build/desktop_shell.elf` (and bundle into disk image).
3. Update `kernel/kernel.c`:
   - Set process name to `"desktop_shell"`.
   - Update embedded fallback Ring 3 bytecode sequence to construct the full Desktop Shell surface (wallpaper background, icons, taskbar, start button, and clock), map it, show it, and run the persistent interactive event loop.

## 2. Why
Migrate the Desktop Shell from Ring 0 to Ring 3 userspace while keeping BWE and GOP hardware authority in Ring 0.

## 3. Expected Result
After post-login handoff, the real physical screen shows the Ring 3 Desktop with wallpaper, taskbar, Start button, and interactive desktop icons.

## 4. Rollback Plan
Revert changes to `kernel.c` and `build.ps1` if regression occurs.
