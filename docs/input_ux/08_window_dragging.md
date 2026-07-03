# Window Dragging and Resizing

## State Machine
The dragging and resizing engine lives inside `BWE_ProcessMouseInteraction`. It behaves as a state machine.

**Variables:**
- `s_is_dragging`, `s_is_resizing`
- `s_drag_win_id`, `s_resize_win_id`
- `s_drag_offset_x/y`, `s_resize_start_x/y`

## Drag Sequence
1. **Start:** User presses MOUSE_DOWN. `BWE_HitTest` returns `BWE_HIT_TITLEBAR`.
2. **Capture:** `s_is_dragging` set to `true`. Offsets calculated to prevent snapping the window to the cursor's top-left corner.
3. **Move:** On subsequent MOUSE_MOVE events (with `buttons != 0`), the new position is calculated. Hardware clamping prevents the titlebar from going offscreen.
4. **Mutate:** `BOS_SetBounds` applies the coordinates.
5. **Release:** MOUSE_UP clears `s_is_dragging`.

## Resize Sequence
Follows a similar capture-move-release pattern. On Move, the bounds `width` and `height` (and `x`/`y` for left/top edges) are manipulated based on the delta from `s_resize_start_x/y`.

## Invalidation & Damage
`BOS_SetBounds` executes `BWE_InvalidateWindow`, marking the parent and all children as `is_dirty = true`. Because the compositor redraws dirty windows, moving a window generates continuous massive damage regions over the entire client rectangle at the polling rate of the mouse.
