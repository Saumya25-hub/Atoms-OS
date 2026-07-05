# Debugging and Telemetry

Window Manager V2 includes built-in telemetry to assist in reaching 60 FPS performance goals and verifying system stability.

## Telemetry Metrics

The compositor tracks:
- `drawn_rects`: The number of individual surface-rectangle intersections drawn per frame.
- `composition_time_ms`: The total time spent in the `compositor_compose()` function (future integration with OS high-precision timer).
- Active Surfaces (derived from tree traversal).
- Active Windows.

## Overlay

These metrics can eventually be fed into a Performance Overlay surface that renders on top of the Desktop, providing real-time visual feedback on composition efficiency and dirty region aggregation.
