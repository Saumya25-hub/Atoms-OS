# Theme Engine

The Theme Engine was heavily expanded in Phase 8.3 to ensure no hardcoded colors exist inside the structural GUI components.

## New APIs

- `theme_draw_taskbar`: Fills the dark gray Taskbar surface.
- `theme_draw_start_button`: Draws the classic blue Start Button and handles Hover/Pressed coloring.
- `theme_draw_desktop`: Paints the teal background.
- `theme_draw_icon`: Renders the highlight boxes for hover/selection states.

By centralizing these calls, future theme packs (e.g. Dark Mode, Windows XP Luna style) can be implemented entirely by modifying `theme_engine.c`.
