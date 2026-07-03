# Layout Engine

## Constants & Constraints
- `MAX_TASK_BUTTONS`: 128 (Increased from 16 to handle massive loads).
- `MIN_BTN_WIDTH`: 40px.
- `START_BTN_WIDTH`: 100px.
- `CLOCK_AREA`: 70px.

## Logic Flow
1. Determine `available_width = panel_width - 170`.
2. Determine `desired_width = 150px` (preferred button width).
3. Check `total_desired_width = active_count * (desired_width + 10)`.
4. If `total_desired_width > available_width`, transition to Dynamic Scaling:
   - `btn_w = (available_width + 10) / active_count - 10`.
5. If `btn_w < 40`, transition to Overflow Mode:
   - `btn_w = 40`.
   - `render_count = (available_width + 10) / 50`.
   - Render `>>` overflow indicator.
