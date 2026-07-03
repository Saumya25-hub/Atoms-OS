# Overflow Strategy

The absolute minimum button width is capped at 40 pixels.

If a user launches an enormous number of applications (e.g. 50 calculators) such that scaling buttons equally to fit within the screen bounds drops `btn_w` below 40px, the Task Panel triggers an **overflow state**.

## Behavior
- `btn_w` locks at `40px`.
- `render_count` is clamped mathematically: `(available_width + 10) / 50`.
- Only the first `render_count` windows in the global registry are displayed.
- The UI prints a `>>` indicator to the right of the last button.

This guarantees the desktop never crashes or bleeds rendering bounds.
