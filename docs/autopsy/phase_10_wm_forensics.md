# PHASE 10 — WINDOW MANAGER FORENSIC AUTOPSY

This autopsy investigates the Window Manager pipeline (from Window Creation to Final Draw List) to prove exactly what is happening to the DOOM window in the compositor's Z-order traversal.

## 1. Does the DOOM window exist?
**YES.**
**Proof:** 
DOOM's runtime invokes `BOS_CreateWindow` via syscall 30 (`SYS_GUI_CREATE_WINDOW`). 
Inside `kernel/wm/bwe/src/bwe_window.c` (line 128), `BWE_AllocateWindowSlot` correctly assigns DOOM a valid slot in `g_windows` and returns a valid `id` (e.g., `4107`). The window struct is zeroed and properly initialized.

## 2. Is it registered?
**YES.**
**Proof:**
When DOOM's surface is created, `BOS_CreateSurface` (in `bwe_window.c`, line 181) executes:
```c
    if (parent_id == BWE_DESKTOP_ID) {
        z_stack_push(id);
    }
```
Since `BOS_CreateWindow` hardcodes `parent_id = BWE_DESKTOP_ID`, DOOM's `id` is successfully pushed into `g_z_order_stack` via `z_stack_push`. `BWE_UpdateZOrders()` is immediately called, which assigns DOOM a layer group of `1` and ensures it sits above the Desktop background (layer group `0`).

## 3. Is it visible?
**YES.**
**Proof:**
DOOM's initialization code calls `BOS_ShowWindow(s_doom_window)`, which fires syscall 35 (`SYS_GUI_SHOW_WINDOW`), landing in `BOS_Show(uint32_t window_id)`.
In `bwe_window.c`, `BOS_Show` explicitly sets `win->state = BWE_STATE_SHOWN`. The state is absolutely not `BWE_STATE_HIDDEN` (which is a different enum value, `4`).

## 4. Is it inside the desktop tree?
**YES.**
**Proof:**
DOOM is a direct child of the Desktop Root.
In `BOS_CreateWindow` (the one in `bwe_window.c` that the linker uses), the function creates the window with:
```c
bwe_error_t err = BOS_CreateSurface(BWE_DESKTOP_ID, x, y, width, height, BWE_WINDOW_RESIZABLE | BWE_WINDOW_MOVABLE, &id);
```
So `win->parent_id` is definitively `BWE_DESKTOP_ID` (which is `0`). 

## 5. Is it traversed during compose?
**YES.**
**Proof:**
In `bwe_compositor.c`, `BWE_ComposeFrame()` executes a bottom-to-top Z-order stack scan:
```c
for (uint32_t i = 0; i < g_z_stack_count; i++) {
    BWE_Window* win = BWE_GetWindow(g_z_order_stack[i]);
```
DOOM's ID (e.g., 4107) exists in `g_z_order_stack` (at index 1 or higher, right above the Desktop Background at index 0). The compositor loops over it.

## 6. Is it skipped?
**NO. DOOM is NOT skipped by the window manager.**
It successfully passes every single rejection branch in `BWE_ComposeFrame`:

* **State Check:** `if (!win || win->state == BWE_STATE_HIDDEN) continue;`
  **Passes:** `win->state` is `BWE_STATE_SHOWN` (3), not `BWE_STATE_HIDDEN` (4).
* **Tree Check:** `if (win->id != BWE_DESKTOP_ID && win->parent_id != BWE_DESKTOP_ID) continue;`
  **Passes:** `win->parent_id` is exactly `BWE_DESKTOP_ID` (0).
* **Occlusion Check:** `if (is_occluded(win, i)) { continue; }`
  **Passes:** DOOM is in layer group 1 and is pushed *after* the Desktop background. There are no full-screen windows in the Z-order above DOOM covering its bounds `(100, 100, 640, 400)`. Thus `is_occluded` returns `false`.
* **Dirty Bounds Check:** `if (win->screen_bounds.x + win->screen_bounds.width < current_dirty.x || ...) { continue; }`
  **Passes:** DOOM submitted the frame, so `current_dirty` *is* exactly DOOM's own bounds `(100,100, 640,400)`. DOOM intersects its own dirty rectangle perfectly.
* **Opacity Check:** `if (win->opacity == 0) continue;`
  **Passes:** `BOS_CreateSurface` sets `win->opacity = 255`.

### Final Execution Proof:
Because it passes all checks, `BWE_ComposeFrame` pushes DOOM's clip rectangle, logs `inst_print_event("Windows Draw");`, and invokes `compose_window_recursive(&ram_fb, win)`. 
In your logs, `[FRAME 503]` logs EXACTLY TWO `Windows Draw` events. 
1. The Desktop Background (which intersects DOOM's dirty rect).
2. The DOOM Window.

Inside `compose_window_recursive`, it finds `win->on_render == bos_canvas_render` and calls it directly. `bos_canvas_render` successfully reads the 32-bit ARGB pixels from `win->control_data.canvas.pixel_buffer` and dumps them directly onto `fb->buffer` inside the `clip` boundary.

## Conclusion

The window manager is doing exactly what it's supposed to do. The Z-order is correct, the bounds are correct, clipping is correct, and the pixel iteration block correctly runs and copies DOOM's memory to the RAM backbuffer.

**Therefore, if DOOM pixels never become visible, it is NOT because the window manager skipped the window.**
