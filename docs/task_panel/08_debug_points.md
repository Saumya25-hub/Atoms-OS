# Debug Points

## Telemetry Variables
The following globals can be read via memory dump or live debugger:
- `g_tp_total_buttons`: Absolute count of targetable windows.
- `g_tp_visible_buttons`: How many fit on screen.
- `g_tp_overflow_count`: How many are hidden.
- `g_tp_panel_width`: Total physical pixel width of the panel.
- `g_tp_free_space`: Pixels remaining before clock.
- `g_tp_layout_passes`: Increments on every layout evaluation.

If UI bleeding occurs, inspect `g_tp_panel_width` against screen bounds to ensure parent bounds were not corrupted.
