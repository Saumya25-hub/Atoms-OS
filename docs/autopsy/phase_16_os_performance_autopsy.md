# BOS OS — Severe Lag / Low FPS Forensic Performance Autopsy

This document provides a forensic analysis of the OS runtime pipeline regarding the severe lag and low FPS, answering the critical questions based on runtime evidence and code inspection.

## PRIMARY ROOT CAUSE:
The primary root cause of the severe lag is **synchronous serial debug logging occurring inside the critical path of the frame rendering loop and input processing loop.** Every frame composite and every input event triggers multiple `display_print` calls, which synchronously write characters to the unbuffered, extremely slow COM1 serial port at 115200 baud. 

## SECONDARY BOTTLENECKS:
1. **Inefficient `HitTest` and Input Event Dispatch:** Every mouse event invokes `BWE_HitTest` across all windows recursively without early-exit spatial optimization (the O(1) fast path cache was `#if 0` disabled).
2. **Total Framebuffer Copy (`SwapFull`):** The `BWE_ComposeFrame` unconditionally executes `BOVISUAL_Graphics_SwapFull(back_vram_ptr)` if there is *any* damage (i.e. `g_dirty_rect_count > 0`), which copies the entire 1920x1080 (8 MB) RAM framebuffer to VRAM on a slow bus.
3. **Timer Loop Underflow Bug:** The main loop timer calculates `elapsed = current_ticks - last_frame_ticks;`. Because it adds `16` to `last_frame_ticks` rather than syncing it strictly to `current_ticks`, `elapsed` underflows to `0xFFFFFFFFFFFFFFFF`, causing the `(elapsed >= 64)` check to evaluate to true, forcing continuous, unthrottled loop execution.

## MEASURED FRAME-TIME BREAKDOWN:
While specific microsecond counters fluctuate based on the volume of events, the fundamental architectural breakdown of a 300+ ms frame (3 FPS) is:
- **Input Processing (`BWE_PumpEvents`):** Dominates time during mouse movement. Processing ~80 `InputEvents/sec` triggers `HitTest` and `[INPUT TRACE]` serial logs (hundreds of slow characters). 
- **Window Composition (`BWE_ComposeFrame`):** `inst_print_event` ("Windows Draw", "BWE_ComposeFrame START", "SwapFull Queue", etc.) writes heavily to the serial port.
- **VRAM Copy (`SwapFull`):** Unconditionally copies the 8 MB RAM framebuffer to VRAM per composed frame, taking a large percentage of PCI bus time.
- **Page Flip / Presentation:** Minimal overhead (hardware page flip `vbe_swap_page`).

## WHY IDLE = ~13 FPS:
Even when "idle", the DOOM application is running and continuously rendering. DOOM updates its window, which registers dirty rectangles. Consequently, `g_dirty_rect_count` is always > 0.
As a result, `BWE_ComposeFrame` does NOT return early. It loops through the dirty regions, runs `SwapFull` (copying 8 MB of VRAM), and synchronously outputs `inst_print_event` telemetry (e.g., "BWE_ComposeFrame START", "SwapFull Queue", "Windows Draw"). At 115200 baud, writing these debug strings block the execution enough to drag the loop down to ~13 FPS, combined with the VRAM copy.

## WHY MOUSE MOVEMENT = ~3 FPS:
When the mouse moves, the system generates dozens of `BV_EVENT_MOUSE_MOVE` events per frame. For each event, `BWE_PumpEvents` executes:
1. `display_print("[INPUT TRACE] Queue Pop\n");`
2. `BWE_HitTest(...)` which iterates all windows.
3. `display_print("[INPUT TRACE] HitTest\n");`
Writing ~45 characters synchronously to the serial port *per mouse event* (at 80 events/sec) fully saturates the CPU waiting for the UART transmit buffer. Furthermore, the mouse adds two dirty rectangles per event (old cursor + new cursor position), leading to redundant partial compositor redraws in the `compose_window_recursive` loop, triggering even more "Windows Draw" serial prints per frame.

## EXACT FILE/FUNCTION RESPONSIBLE:
1. `kernel/drivers/display/display.c` -> `display_print()`: Unbuffered loop over `serial_write()`.
2. `kernel/wm/bwe/src/bwe_core.c` -> `BWE_PumpEvents()`: Logs `[INPUT TRACE]` for every event and executes O(N) HitTest.
3. `kernel/wm/bwe/renderer/bwe_compositor.c` -> `BWE_ComposeFrame()`: Emits `inst_print_event()` heavily and unconditionally calls `BOVISUAL_Graphics_SwapFull(back_vram_ptr)`.
4. `kernel.c` -> `kernel_main()` (Main Loop): Broken `elapsed` timer logic allowing unbound execution.

## INVESTIGATION RESPONSES:
- **Redundant Windows Draws:** In `BWE_ComposeFrame`, the compositor iterates over every dirty rect and redraws windows intersecting it. Because the mouse leaves an old rect and creates a new one, the same window intersecting both rects is drawn twice, triggering `inst_print_event("Windows Draw")` multiple times.
- **Dirty Rect optimization bypassed:** `g_dirty_rect_count` is checked to skip the frame, but when >0, the compositor falls back to executing `SwapFull` unconditionally, rendering the VRAM transfer completely unoptimized. Also, `inst_print_val` logs `g_dirty_rect_count` as 0 because it prints *before* dirty rects are actually evaluated and queued for the frame.

## RECOMMENDED PRODUCTION FIX:
1. **Disable Synchronous Serial Tracing:** Wrap `display_print` and `inst_print_event` calls inside `#ifdef DEBUG_TRACE` or implement a buffered, asynchronous serial logger. Remove `[INPUT TRACE]` prints from the hot path in `BWE_PumpEvents`.
2. **Re-enable Partial VRAM Copy:** Remove the unconditional `BOVISUAL_Graphics_SwapFull` from `BWE_ComposeFrame` and allow the BSPE Dual-Page Present engine to only copy merged dirty rectangles.
3. **Fix Timer Underflow:** Fix the main loop frame-pacing logic in `kernel.c` by ensuring `last_frame_ticks` never advances past `current_ticks`, preventing the `elapsed` variable from underflowing. 
4. **HitTest Caching:** Restore O(1) window lookup for pointer events instead of traversing the UI tree per event.
