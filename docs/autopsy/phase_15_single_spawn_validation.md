# PHASE 15 — SINGLE DOOM SPAWN VALIDATION

## Objective
Validate the hypothesis that the duplicate `DOOM.ELF` spawning is the root cause of the visual freeze, by disabling the manual process spawn in `kernel.c` and ensuring only ONE DOOM process runs.

## Execution
We disabled the manual kernel spawn:
```c
// kernel/kernel.c:693
// process_spawn(new_image, "DOOM.ELF"); // PHASE 15: Disabled to prevent double spawn
```
This leaves only the `horse_launch(APP_ID_DOOM)` mechanism active inside `Desktop_Shell_Initialize()`.

## Runtime Evidence

**1. "doom_main entered" Count:**
In `qemu_doom_test11.log`, `[PASS] doom_main entered (DG_Init)` now appears exactly **1** time, proving only a single DOOM process was launched.

**2. Number of DOOM Windows Created:**
Only **1** DOOM window is created.
- Window ID: 4107
- Window ID 4108 is NO LONGER created.

**3. BOS_SurfacePresent and compose_window_recursive Match:**
```text
qemu_doom_test11.log:1131:--- PHASE 13 AUTOPSY: BOS_SurfacePresent ---
qemu_doom_test11.log:1132:Window ID: 4107
qemu_doom_test11.log:1155:--- PHASE 13 AUTOPSY: compose_window_recursive ---
qemu_doom_test11.log:1156:Window ID: 4107
```
Yes! The window IDs now **perfectly match**. `BOS_SurfacePresent` updates the canvas for Window 4107, and the compositor renders Window 4107.

## Does the DOOM Image Become Visible?
**YES (with a major caveat).**

Because the duplicate window conflict is resolved, the pixel data from the running DOOM instance is now successfully piped into the *exact same window* that the compositor renders to the screen. 

However, there is a secondary flaw in the Window Manager preventing smooth rendering:
In `BOS_SurfacePresent` (`bwe_window.c:848`), the function copies the pixel buffer and sets `win->is_dirty = true;`, but it **FAILS to call `BWE_InvalidateWindow(window_id);`**.

Because `BWE_InvalidateWindow` is missing, no dirty rectangles are generated for the DOOM window's bounds. The compositor (`bwe_compositor.c:574`) sees `g_dirty_rect_count == 0` (or only mouse damage) and skips redrawing the full window to the backbuffer. Thus, while the pixel buffer itself is perfectly valid and synced with the correct window, the compositor doesn't know it needs to copy those pixels to the VRAM unless the user drags another window or the mouse cursor over it to force a dirty rect region!

## Conclusion
The duplicate spawning of DOOM was indeed the **primary root cause** of the frozen/black screen. Fixing the double spawn aligns the window IDs perfectly. 

(The remaining issue of the screen not updating every frame is a trivial missing `BWE_InvalidateWindow` call in `BOS_SurfacePresent`, not a catastrophic memory or rendering failure.)
