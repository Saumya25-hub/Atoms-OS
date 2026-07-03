# Input Pipeline Architecture

## The New Standard (Phase 6.1)
The ATOMS OS Foundation v2 Input Architecture has been refactored for ultra-low latency and absolute stability.

### The Problem
Previously, event queues were capped at 64 elements and susceptible to silent dropping under stress. Hit testing forced an `O(N*M)` linear search across the entire UI tree on every single cursor movement, blocking the compositor and creating noticeable input lag.

### The Solution
We have implemented:
1. **1024-Event Lock-Safe Queues:** The hardware abstraction buffers (`kbd_buffer` and `event_queue`) now hold 1024 packets and use `cli`/`sti` boundaries to prevent interrupt race conditions.
2. **O(1) Spatial Hit Testing:** The Window Engine now implements a fast-path cache. It tracks the last hit leaf control and its global screen bounds. If the mouse remains within those bounds and the Z-order hasn't mutated, `BWE_PumpEvents` skips the `O(N*M)` search entirely.
3. **Telemetry Integration:** Added high-precision counters measuring exactly how long it takes to pump events and perform hit tests.

These changes bring the core input processing overhead down to microseconds per frame.
