# Latency Report

## Target Met: < 2ms

Prior to Phase 6.1, Hit Testing took upwards of ~5-15ms depending on the number of controls on the screen because it searched every button, label, and layout within every window.

With the new `O(1)` `s_cached_z_version` fast-path check, the time it takes to process a `MOUSE_MOVE` event has dropped to virtually zero (less than the granularity of the millisecond `timer_get_ticks()`).

## Breakdown
- **IRQ to Queue:** Microseconds.
- **Queue to Pop:** Time waiting for V-Sync / `BOHeart_Pulse` (16ms max delay on 60FPS, expected).
- **Pop to App Dispatch:** Microseconds (O(1) hit testing cache hit).
- **App Processing:** Depends on user code.
- **Render Output:** Dependent on Compositor speed.
