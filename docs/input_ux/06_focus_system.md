# Focus System Architecture

## Focus Ownership
Focus determines which window receives keyboard events.
State is maintained globally in `bwe_core.c` via:
- `g_focused_window_id`: The window receiving keyboard input.
- `g_active_window_id`: The visually active window (often the same as focus).

## Transitions
When `BOS_SetFocus(window_id)` is invoked:
1. Validates the target window ID.
2. Fetches the currently focused window (`old`).
3. Sets `old->state = BWE_STATE_DEACTIVATED`.
4. Invalidates the old window (forcing a redraw).
5. Dispatches `BWE_EVENT_FOCUS_LOSS` to `old->on_event`.
6. Updates global variables to `window_id`.
7. Sets target `state = BWE_STATE_ACTIVE`.
8. Invalidates the target window.
9. Dispatches `BWE_EVENT_FOCUS_GAIN` to the target.
10. Automatically calls `BWE_BringToFront(window_id)` to mutate the Z-order.

## Critical Flaw
Focusing a window synchronously mutates the Z-order stack. `BWE_BringToFront` relies on `BWE_UpdateZOrders` which executes a literal Bubble Sort. Rapidly clicking windows will spike CPU usage exponentially based on the total window count.
