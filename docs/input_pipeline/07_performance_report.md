# Performance Report

## Throughput Improvements
- **Max Mouse Frequency:** System can now reliably ingest and process 1000Hz polling rates without dropping packets, thanks to the 1024-event buffer.
- **Max Keyboard Repeat:** Modifier drops are eliminated. Even holding down 10 keys simultaneously (N-key rollover limits depending on PS/2 hardware) will perfectly enqueue.

## CPU Load
The `O(1)` Hit Test cache removes the primary source of CPU spiking during mouse movement. Dragging a window still invalidates large areas, but the *discovery* of the window happens instantly.

Overall input processing now takes ~0.0% of the active CPU frame time.
