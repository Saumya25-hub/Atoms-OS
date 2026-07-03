# Performance

The Task Panel Layout Engine executes completely in `O(N)` time relative to the number of active windows.

## Two-Pass Guarantee
1. The first loop counts windows (`O(N)`).
2. The second loop draws (`O(M)` where `M <= render_count`).

Because `MAX_TASK_BUTTONS` is capped at 128, and rendering only issues primitive `BWE_FillRect` and `BWE_DrawText` calls, the layout calculation is imperceptible on CPU metrics.

There is no heap allocation during this pipeline. `active_win_ids` uses stack memory.
