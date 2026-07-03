# Render Flow

The `task_panel_render_callback` executes exclusively when the Window Manager sends an invalidation signal to the `g_task_panel_win_id`.

## Pipeline
1. Draw solid background `#0F172A`.
2. Draw Start Button `#2563EB` (or `#1E293B` if closed).
3. Compute dynamic layout passes.
4. Loop through `render_count` tasks:
   - Calculate background: `focused ? #334155 : #1E293B`.
   - Truncate text using `task_panel_truncate_text`.
   - Issue `BWE_FillRect` and `BWE_DrawText`.
5. Draw Clock using standard `read_rtc_time`.
6. Print Overflow warning `>>` in `#EAB308` if applicable.
