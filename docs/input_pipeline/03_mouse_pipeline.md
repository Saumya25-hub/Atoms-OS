# Mouse Pipeline Optimization

## Latency Reduction
The mouse pipeline translates X/Y deltas into absolute screen coordinates. 
In Phase 6.1, we introduced `g_mouse_events_per_sec` to track throughput.

The most significant latency reduction for the mouse comes not from the driver, but from `BWE_PumpEvents`. Because the mouse triggers a `BWE_EVENT_MOUSE_MOVE` on every sub-pixel shift, `BWE_HitTest` was destroying performance.

The new `O(1)` fast-path cache checks:
1. Did the Z-order version (`g_z_order_version`) change?
2. Is the mouse inside the cached `target_win->screen_bounds`?
If both are true, it instantly returns the cached window ID, eliminating recursive traversal and reducing dispatch time to near zero.
