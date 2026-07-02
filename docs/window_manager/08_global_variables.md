# Global Variables

The current Window Manager suffers from heavy reliance on global state, making it highly susceptible to race conditions.

## BWE Core (`bwe_core.c`)
- `g_windows[1024]`: The static master pool of all BWE Windows.
- `g_window_generations[1024]`: Tracks ID recycling to prevent stale pointer access.
- `g_focused_window_id`: The ID of the currently focused window. Read by input router, modified by `BOS_SetFocus()`.
- `g_bwe_mouse_x` / `g_bwe_mouse_y`: Global tracking of the mouse. Dangerous if accessed during an interrupt context while rendering.
- `g_kernel_screen_width` / `height`: Screen resolution imported from the VBE driver.

## BOSurface (`surface.c`)
- `surface_pool[128]`: A redundant pool? Represents the lower-level compositor surfaces.
- `bwe_is_dragging`: Boolean flag locking the WM into a drag state.
- `bwe_drag_surface_id`: The ID of the window currently being dragged.
- `bwe_drag_offset_x` / `y`: The delta between the mouse cursor and the window's top-left corner.
- `g_update_lock`: A rudimentary spinlock/mutex attempt to prevent concurrent rendering. Very risky.
- `bwe_capture_surface_id`: The window that has "captured" the mouse (e.g., holding down a scrollbar).

## BOCompositor (`bocompositor.c`)
- `g_stats`: Collects rendering performance metrics (occlusions, clips).
- `g_render_callback`: A function pointer invoked when the compositor is ready to draw a surface.
- `g_compositor_current_surface_id`: Tracks which surface is actively being rendered in the current batch loop.

## Desktop Shell (`desktop_shell.c`)
- `s_app_registry[16]`: Static array of registered applications.
- `g_wallpaper_style` / `g_wallpaper_bg_color`: Configures the desktop background.
- `g_start_menu_win_id`: Global reference to the Start Menu window to easily toggle its visibility.

## Risk Assessment
The pervasive use of globals like `bwe_is_dragging` and `g_focused_window_id` across multiple files without strict mutex locking means that if a UI thread preempts a render thread, severe visual or memory corruption could occur.
