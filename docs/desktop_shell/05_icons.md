# Desktop Icons

The Desktop Icons are lightweight interactive objects painted directly onto the Desktop Root surface. They do not own their own framebuffers, minimizing memory footprint.

## Lifecycle

Icons are registered via `desktop_icon_add()`. They store an ID, Name, coordinates, and a click callback.

## Hit Testing

During `desktop_shell_hit_test()`, the icon array is scanned.
If the mouse falls within the 64x64 bounding box of an icon:
1. `is_hovered` becomes true, invalidating the icon's rect to trigger a redraw with a semi-transparent white box.
2. If clicked, `is_selected` becomes true, triggering a redraw with a semi-transparent blue box.
3. Callbacks (like launching an app) can be mapped here.
