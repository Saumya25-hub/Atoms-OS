# Performance Report

## Core Metrics (Estimated based on Architecture)

| Metric | Estimation | Bottleneck Factor |
|---|---|---|
| **Mouse Polling Latency** | 3 IRQs per packet | High CPU Interruption |
| **Input Queue Pressure** | 64 limit / frame | Likely to drop fast movements |
| **Event Routing (Hit Test)** | `O(N * M)` depth | Severe UI stall during hovering |
| **Focus Switching** | `O(N^2)` Sort + Full Invalidate | Severe stutter on click |
| **Window Dragging Damage** | `W * H` per tick | Massive VRAM / memcpy bottleneck |
| **Cursor Latency** | Coupled to VSync/Compositor | Feels "heavy" if UI is busy |

## Why the Mouse Feels Laggy
1. **Interrupt Storms:** The mouse driver reads one byte per interrupt, requiring 3 interrupts to form a packet, starving the kernel.
2. **Synchronous Pump:** Events are pumped inside `BOHeart_Pulse` just before rendering. If rendering takes 16ms, events queue up and lag by 16ms.
3. **Expensive Hit Testing:** `BWE_PumpEvents` evaluates every pixel coordinate against every window's 8-way bounding box, deep into the child tree, every time the mouse moves.
4. **Full Window Redraws:** Moving a window sets `is_dirty = true` recursively. If you drag a 800x600 window, the compositor copies 1.9MB of pixel data over the system bus on *every pixel of movement*.
