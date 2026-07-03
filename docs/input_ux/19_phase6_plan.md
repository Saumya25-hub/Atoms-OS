# Phase 6 Stage 2 Execution Roadmap

## Objective
To implement the recommendations from the Deep Input & UX Architecture Audit without disrupting the existing ATOMS OS compatibility.

## Execution Sequence

### Phase 6.2.1: Queue Hardening
1. Modify `input.h`: Increase `MAX_EVENTS` from 64 to 1024.
2. Modify `keyboard.c`: Ensure `kbd_buffer` is properly protected by local `cli/sti` blocks when pushing and popping to prevent ring corruption.

### Phase 6.2.2: The Z-Order Refactor
1. Open `bwe_window.c`.
2. Delete the Bubble Sort algorithm in `BWE_UpdateZOrders`.
3. Implement a proper insertion sort or sorted-insertion algorithm.
4. Result: `O(N)` or `O(N log N)` focus switching instead of `O(N^2)`.

### Phase 6.2.3: Damage Control (Dirty Rectangles)
1. Modify `bwe_core.c` and `bwe_window.c` to track an `is_dirty` rectangle instead of a boolean flag.
2. In `BOS_SetBounds`, calculate the union of `old_screen_bounds` and `screen_bounds`. Invalidate only that region.
3. Update `BWE_ComposeFrame` to accept a clipping rectangle array, preventing full-screen redraws.

### Phase 6.2.4: Spatial Caching
1. Instead of executing `BWE_HitTest` on every leaf child dynamically, maintain a 2D bounding grid for rapid lookup.
2. If grid is too complex for this OS stage, at least cache the target window `screen_bounds` explicitly so we skip checking children if the mouse is totally outside the parent.

### Phase 6.2.5: Cursor Decoupling
1. Investigate if the current VESA VBE implementation supports a hardware cursor layer.
2. If not, isolate cursor draw to the very final step of `BOHeart_Pulse` and draw it directly to VRAM, backing up the pixels beneath it to a small buffer. On move, restore the backup buffer, update position, and redraw. Do not rely on the global compositor to redraw the cursor.

## Strict Rules for Implementation
- **DO NOT** delete the `bwe_core` or `input_abstraction` compatibility APIs.
- **DO NOT** rewrite the PS/2 driver; just fix the drop rates.
- Maintain fallback compatibility for all user applications.
