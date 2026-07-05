# Window Object

A `BOSWindow` acts as the bridge between OS processes and the graphical representation (Surface).

## Architecture

- **State**: Tracks Normal, Hidden, Minimized, Maximized states.
- **Constraints**: Stores min/max dimensions.
- **Ownership**: Tracks the `owner_pid` to automatically tear down surfaces when a process crashes or exits.

## Window Manager API

Applications interact with windows through the Window Manager API, never touching the framebuffer directly:

- `window_create()`
- `window_destroy()`
- `window_show()`
- `window_hide()`
- `window_move()`
- `window_resize()`
- `window_focus()`
- `window_close()`

When a window moves or resizes, the Window Manager automatically calculates the old and new dirty rectangles and pushes them to the Compositor's invalidation queue.
