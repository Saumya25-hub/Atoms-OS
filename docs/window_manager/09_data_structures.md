# Data Structures

## `BWE_Window` (`bwe.h`)
The "God Object" of the UI subsystem. A massive struct (over 100 bytes) that represents *everything* from a top-level window to a tiny checkbox.
- **Hierarchy:** `id`, `parent_id`, `children[128]`, `sibling_index`. (128 children max per window is a hard limit).
- **Geometry:** `local_bounds`, `screen_bounds`, `min_size`, `max_size`.
- **Layout:** `margins`, `padding`, `dock_mode`.
- **State:** `type` (Window, Panel, Button), `flags`, `opacity`, `is_dirty`.
- **Control Data (`union`)**: A gigantic union storing state for Buttons (text, colors), Textboxes (cursor pos), Scrollbars (values), etc. *Impact:* Every small label consumes as much memory as a complex treeview.
- **Callbacks:** `on_event`, `on_render`.

## `BOCompositorSurface` (`compositor_types.h`)
A highly optimized, stripped-down representation used strictly for rendering math.
- `id`: Matches the `BWE_Window` ID.
- `x, y, width, height`: Absolute screen bounds.
- `z_order`: Sorting integer.
- `clip_rect` / `invalid_rect`: Used for damage tracking.
- `flags`: Opaque, Focused, Visible.
- `surface_handle`: Opaque pointer back to the `BWE_Window`.

## `BWE_Rect` / `BOCompositorRect`
Standard rectangles.
- `x`, `y`
- `width`, `height`

## `BOCompositorStats`
Diagnostic structure tracking rendering performance.
- `visible_windows`: How many windows survived occlusion.
- `dirty_rectangles`: How many areas were redrawn.
- `occlusion_count`: How many windows were culled.
- `batch_count`: Total draw calls executed.

## `ShellAppEntry` (`desktop_shell.h` implied)
Used by the Taskbar / Start Menu.
- `display_name`
- `init_callback`: Function pointer to launch the app.
- `category`: String representing the app group.
