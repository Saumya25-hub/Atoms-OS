# Render Pipeline

The entire rendering pipeline is governed by `BOCompositor` interacting with the `BVFramebuffer`. Rendering is deferred and event-driven via Dirty Rectangles (Damage).

## 1. Trigger Phase
Any action (mouse hover, text typing, window move) calls `BWE_InvalidateWindow()`, which pushes a dirty rectangle via `BOCompositor_AddDamage()`.

## 2. Occlusion Pass
`BOCompositor_ComposeFrame()` sorts all visible surfaces by `z_order` (Back to Front). Before drawing, it sweeps from Front to Back (highest Z to lowest Z). If it finds an opaque surface that completely overlaps a surface beneath it, the surface beneath is marked `occluded = true` and entirely skipped during rendering.

## 3. Back-to-Front Draw Pass
The compositor iterates the sorted list (Back to Front):

1. **Desktop (Z-Order = 0):** `kernel/shell/desktop_shell.c` receives a render callback. `Shell_DrawWallpaper()` fills the background.
2. **Standard Windows (Z-Order 1 to N):** 
   - Shadows are drawn around bounds (`BWE_DrawShadow`).
   - Window borders are drawn (`BWE_DrawBorder`).
   - Titlebars and buttons are drawn (`BWE_DrawTitleBar`).
   - The Window's specific client area `on_render` callback fires.
3. **Topmost Windows (Z-Order N+1):** Taskbar (`taskbar.c`) and Start Menu are rendered on top of standard windows.

## 4. Hardware Overlay
**Mouse Cursor:** The mouse cursor is not a `BWE_Window`. It is tracked by `mouse_engine.c` and drawn directly to the framebuffer as a hardware-level sprite overlay, completely bypassing `BOCompositor` occlusion logic to guarantee zero-latency input rendering.

*Note: Desktop Icons are currently integrated into the Desktop Shell's render callback, drawn immediately after the wallpaper.*
