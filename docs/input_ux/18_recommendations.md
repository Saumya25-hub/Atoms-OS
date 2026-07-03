# Recommendations

The following actionable improvements form the basis of Phase 6 Stage 2.

## 1. Hardware Cursor Overlay
- **Problem:** Cursor feels laggy.
- **Root Cause:** Cursor rendering is tied to compositor pipeline.
- **Priority:** HIGH
- **Fix:** Implement a hardware cursor via VBE/VGA registers if possible, or decouple cursor rendering to a fast-path overlay buffer that bypasses the window compositor.

## 2. Event Queue Expansion & Lockless Ring Buffer
- **Problem:** Mouse skips during heavy load.
- **Root Cause:** 64-event limit in `input.c` and potential race conditions.
- **Priority:** CRITICAL
- **Fix:** Increase queue to 1024. Implement a proper lockless ring buffer or disable interrupts during `push`/`pop`.

## 3. Spatial Hashing / Quad-Tree for Hit Testing
- **Problem:** UI stalls when mouse moves over many windows.
- **Root Cause:** `BWE_HitTest` iterates every window and recurses every child linearly `O(N*M)`.
- **Priority:** HIGH
- **Fix:** Pre-compute a 2D spatial grid (or bounding volume hierarchy) when windows move. Hit test against the grid in `O(1)` or `O(log N)`.

## 4. Sub-Rectangle Dirty Clipping
- **Problem:** Dragging a window lags the machine.
- **Root Cause:** `BOS_SetBounds` invalidates the entire window and all children.
- **Priority:** CRITICAL
- **Fix:** Calculate exactly which sub-rectangles changed (the delta). Only invalidate and redraw the exposed background and the new edges, not the inner client body.

## 5. Z-Order Sorting Refactor
- **Problem:** Clicking a window spikes CPU.
- **Root Cause:** Bubble sort in `BWE_UpdateZOrders`.
- **Priority:** MEDIUM
- **Fix:** Since it's nearly sorted, use Insertion Sort, or maintain the list in sorted order intrinsically rather than re-sorting.
