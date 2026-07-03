# Rendering Relationship

## Pipeline Breakdown

```text
Input (Mouse/Keyboard IRQ)
↓
Event (Hardware mapped to Event Queue)
↓
Window (Hit Test & Focus updates state bounds and flags)
↓
Damage (BWE_InvalidateWindow sets `is_dirty = true` recursively)
↓
Compositor (BWE_ComposeFrame scans tree for dirty windows and recomposes)
↓
Framebuffer (Final pixels blasted to VRAM)
```

## Impact of Input on Rendering
Because `BWE_InvalidateWindow` forces a boolean `is_dirty = true` on the target window *and all of its descendants*, the relationship between input and rendering is disproportionately heavy.
- A single pixel of mouse drag causes the entire window and all internal UI controls to redraw.
- There is no sub-rectangle clipping inside a window. The compositor evaluates whole surfaces.
- A hover state change (`BWE_EVENT_MOUSE_ENTER`) dirties the entire control, even if only a subtle color change occurs.

## Cursor Re-paint
Because the cursor is likely rendered on the final compositor pass over the dirty regions, lagging the compositor directly lags the cursor. Separating the cursor to a hardware layer or a specialized fast-path overlay would break this dependency.
