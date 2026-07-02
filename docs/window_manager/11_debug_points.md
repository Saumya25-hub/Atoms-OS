# Debug Points

## Where should debugging happen?

1. **`BWE_GetWindow(uint32_t)`**
   - **Why**: This is the most frequently called function in the UI. If an invalid ID is passed, it returns NULL.
   - **What to log**: Whenever it returns NULL for a non-zero ID, it implies a use-after-free or a stale ID bug.

2. **`BOCompositor_ComposeFrame()`**
   - **Why**: This is the render loop. If the system hangs, it's usually here (e.g., an infinite loop in the occlusion pass).
   - **What to log**: `g_stats.compose_time_ms`. Any frame taking > 16ms should assert a warning.

3. **`BWE_InvalidateWindow(uint32_t)`**
   - **Why**: This triggers the recursive damage tracker.
   - **What to log**: If this is called in an infinite loop (e.g., a render callback accidentally invalidates itself), it will lock up the OS.

## Critical Assertions (Missing from current code)
- Assert that `surface_pool` slots are never leaked.
- Assert that `BOCOMPOSITOR_MAX_SURFACES` (128) is never exceeded.
- Assert that mouse coordinates (`g_bwe_mouse_x`) never exceed `g_kernel_screen_width`.
- Assert `g_update_lock` is always released if acquired.

## Risky Areas to Monitor
- The `bodebug_dump()` function currently logs to the console every time the focus or process changes. This is good, but it relies on reading the `Task*` from the scheduler, crossing boundaries between UI and Kernel Core.
- Mouse capture (`bwe_capture_surface_id`). If the window capturing the mouse crashes or forgets to release it on `MOUSE_UP`, all mouse input to the OS is permanently deadlocked.
