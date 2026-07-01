# BWE V2.0 — Input, Compositor & Ghosting Trails Fix Documentation

This document logs the details of the investigation, root causes, and architectural fixes applied to **BOSurface Window Engine (BWE) V2.0** to resolve the frozen mouse cursor, window dragging trails, child widget coordinate desynchronization, and drop-shadow streak artifacts.

---

## 1. Bug: Mouse Cursor Frozen at (0, 0)
* **Symptom**: The mouse cursor was visible on screen but remained static at coordinates `(0, 0)` and did not move or click.
* **Root Cause**: 
  - The OS kernel input loop (`kernel.c`) was pushing raw hardware input events to the queue, but BWE V2.0 did not poll or consume the unified absolute coordinate state from the hardware normalizer (`input_get_latest_state()`).
  - The BWE event queue was never dispatched to window handlers.
* **Resolution**:
  - Updated `BOHeart_Pulse` in [bwe_core.c](file:///d:/Signatures_OS/kernel/bwe/src/bwe_core.c) to fetch mouse coordinate updates using `input_get_latest_state()` and synthetically dispatch `BV_EVENT_MOUSE_MOVE` events.
  - Added a hit-test event router in `BOS_ProcessEvent` to dispatch mouse actions to the topmost widget under the cursor using `BWE_HitTest` and keyboard inputs to the focused window target.

---

## 2. Bug: Window Drag Trails (Cascading Window Duplicates)
* **Symptom**: Dragging a window left permanent staircase-like duplicate copies of the window frame across the desktop background.
* **Root Causes**:
  1. **Double-Buffering Page Flipping Desynchronization**: The system uses hardware double-buffered page flipping. A partial copy of only the current frame's dirty rectangles left the hidden page containing stale data from two frames ago, which became visible on the next page swap.
  2. **Event Coalescing Bounds Overwrites**: Because multiple mouse events run in a single tick before composition, `BOS_SetBounds` was continuously overwriting `win->old_screen_bounds`, losing track of the actual starting position from the end of the previous frame.
* **Resolution**:
  - **Full-Frame VRAM Copy**: Swapped the partial dirty region copy inside `BWE_ComposeFrame` in [bwe_compositor.c](file:///d:/Signatures_OS/kernel/bwe/renderer/bwe_compositor.c) with a single unrolled 64-bit copy call `BOVISUAL_Graphics_SwapFull(&back_vram)`. This completely overwrites VRAM with the up-to-date RAM backbuffer, erasing any trailing pixels from previous pages.
  - **Persistent Bounds Tracking**: Implemented static tracking arrays `s_last_composed_bounds` and `s_last_composed_bounds_valid` in `BWE_ComposeFrame`. The compositor now compares against the actual final state from the previous frame to calculate the clean start-to-finish invalidation rectangles, resolving event coalescing gaps.

---

## 3. Bug: Child Widget Placement Desynchronization
* **Symptom**: When dragging the parent window, the internal control widgets (checkboxes, panels, progress bars, etc.) remained locked at their old coordinate positions on screen and did not move with the window body.
* **Root Cause**: 
  - `BOS_SetBounds` only updated the parent window's `screen_bounds`, leaving the children's absolute coordinates untouched until the next layout trigger.
* **Resolution**:
  - Wired a recursive layout calculation call `BWE_UpdateLayout(window_id)` inside `BOS_SetBounds` in [bwe_core.c](file:///d:/Signatures_OS/kernel/bwe/src/bwe_core.c#L198-L203). This recalculates and adjusts the absolute screen bounds of all child widgets recursively whenever the parent window is dragged or resized.

---

## 4. Bug: Thin Vertical & Horizontal Shadow Streak Trails
* **Symptom**: Moving a window left thin black lines/vertical bars on the right and bottom sides of the window path.
* **Root Cause**:
  - The drop-shadow drawing utility `BWE_DrawShadow` in [bwe_paint.c](file:///d:/Signatures_OS/kernel/bwe/renderer/bwe_paint.c#L150) draws a translucent drop-shadow extending `8px` outside of the window's bottom and right edges.
  - The compositor's invalidation logic was only tracking `win->screen_bounds`, leaving the shadow's `8px` footprint uncleared on vacated regions.
* **Resolution**:
  - Inflated the dirty bounds invalidation rectangles in `BWE_ComposeFrame` by `+8px` width and height for decorated windows:
    ```c
    if (win->id != BWE_DESKTOP_ID && !(win->flags & BWE_WINDOW_BORDERLESS)) {
        rect.width += 8;
        rect.height += 8;
    }
    ```
    - This ensures the drop-shadow footprint is fully covered, allowing the compositor to clear it with the desktop color.

---

## 5. Bug: Close (X) Click Leaves Stale Frame & Mouse "Draws" Navy Blue Blocks
* **Symptom**: Clicking the title bar close (X) button destroyed the window, but the window frame remained visible on screen. When moving the mouse over the dead window, it started "drawing" navy blue rectangular blocks (erasing the window under the mouse path).
* **Root Cause**:
  - `BOS_DestroySurface` successfully updated `win->state = BWE_STATE_DESTROYED` and removed the window from the Z-order list.
  - However, because the window was now `BWE_STATE_DESTROYED`, the compositor's dirty bounds loop was skipping it.
  - Since the compositor skipped it, the area where the window was previously drawn was never marked dirty or recomposited.
  - When the mouse moved, only the mouse's path was marked dirty. Recompositing the mouse path over the dead window redrew the desktop (navy blue) since the window was no longer in the Z-stack, creating a "mouse drawing" erasure trail.
* **Resolution**:
  - Added an invalidation fallback inside the compositor's window dirty check loop: if a slot's window state is `BWE_STATE_DESTROYED` but its `s_last_composed_bounds_valid[i]` is still `true`, its last composed bounds (inflated by shadow sizes) are immediately added to the dirty list to force a redraw, and the validity flag is reset to `false`.
