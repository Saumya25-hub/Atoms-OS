# Telemetry Mechanics

## Exposed Variables
We have defined global telemetry variables that can be integrated into the desktop shell or `bodebug_dump()` module.

1. `g_mouse_events_per_sec`: Increments every time the mouse pushes a movement or click.
2. `g_kbd_events_per_sec`: Increments every time a key scancode is processed.
3. `g_hit_test_time_us`: Measures the internal microsecond (approximated) time it takes to resolve `BWE_HitTest`.
4. `g_pump_time_us`: Measures the total duration of `BWE_PumpEvents()`.

These variables are updated synchronously and can be printed to the screen via `BOS_SetText` if a diagnostics window is requested.
