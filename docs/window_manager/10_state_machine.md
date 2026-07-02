# State Machine

The Window Manager operates across multiple intertwined state machines.

## Lifecycle States (`BWE_SurfaceState`)
- **CREATED**: `BOS_CreateWindow()` called. Slot allocated, memory zeroed. Not yet registered with Compositor.
- **INITIALIZED**: Internal structures populated.
- **SHOWN**: Added to `BOCompositorStack`. Rendering is now active. Damage pushed for the full bounds.
- **HIDDEN**: Removed from `BOCompositorStack`. Previous bounds marked as damaged so the Desktop can redraw over it.
- **ACTIVE**: Reserved for later? Mostly handled via `BWE_STATE_SHOWN` combined with the `BOCOMPOSITOR_FLAG_FOCUSED` flag.
- **DESTROYED**: ID is invalidated, slot is freed, generation bumped. The `BWE_Window` struct is effectively dead.

## Dirty State (`is_dirty`)
- **Clean (`false`)**: No changes since last frame. Skipped during invalidation checks.
- **Dirty (`true`)**: The window has moved, resized, or its content changed. Invalidation is recursive: marking a parent dirty marks all children dirty (`invalidate_descendants_recursive`).

## Drag State (`bwe_drag_state`)
1. **DRAG_IDLE**: Normal operation.
2. **DRAG_DRAGGING**: Triggered when a `MOUSE_DOWN` event hits `BWE_HIT_TITLEBAR`. The mouse captures the window. `bwe_drag_offset` is calculated. All subsequent `MOUSE_MOVE` events are forced to this window.
3. **DRAG_RELEASE_PENDING**: When `MOUSE_UP` occurs, it snaps back to `DRAG_IDLE`.

## Focus State
Focus is managed by `g_focused_surface_id` and tracked in BOCompositor via `BOCOMPOSITOR_FLAG_FOCUSED`.
- Only one window can have focus.
- Clicking *anywhere* on a window transfers focus to it.
- The previous focused window is explicitly invalidated to force a redraw (usually to turn its Titlebar from Active to Inactive colors).
