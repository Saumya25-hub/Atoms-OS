# Input & UX Architecture Walkthrough

## The Goal
This document concludes Phase 6 Stage 1. An extensive audit has been performed across the Mouse, Keyboard, Input Queues, Event Routing, Window Dragging, Focus System, and Rendering Pipeline. 

## How the Entire Input System Works
Hardware interrupts (IRQ 1 for Keyboard, IRQ 12 for Mouse) fire on peripheral activity. The PS/2 drivers translate these electrical bytes into primitive values (X/Y deltas, button states, scancodes) inside an Interrupt Service Routine (ISR).
These are immediately pushed into an abstraction queue (`event_queue` in `input.c`).
During the main kernel graphical loop (`BOHeart_Pulse`), the Window Engine (`BWE`) pops these events. It evaluates the exact screen coordinate against a Z-ordered stack of all windows (`BWE_HitTest`), determines what UI control the mouse is over, and triggers hover/click states. When moving windows, it alters their coordinates and flags the entire window (and its children) as "dirty". The Compositor then redraws all dirty windows to the framebuffer.

## Where Latency Comes From
1. **O(N) Hit Testing:** The kernel asks *every* pixel coordinate if it intersects with *every* window and control on the screen, repeatedly.
2. **Synchronous Pumping:** Events queue up until the renderer is ready to consume them.
3. **Massive Overdraw:** Dragging a window tells the compositor "please redraw this entire application and its background", flooding the memory bus.

## Where Instability Comes From
1. **Tiny Event Queues:** A 64-event buffer is easily overrun by a fast mouse flick, permanently losing movement data.
2. **Race Conditions:** `push_event` non-atomically modifies the queue pointers during hardware interrupts.
3. **Z-Order Sorting:** Bubble sorting the window stack on every focus switch consumes extreme CPU time as the window count grows.

## What Should Be Fixed First
1. **The Event Queue Limits:** Bump queue arrays to `1024` and wrap them in proper lockless atomic or `cli/sti` blocks. This immediately stops dropped inputs.
2. **Cursor Rendering Decoupling:** Stop relying on the global compositor to draw the cursor.
3. **Dirty Rectangles:** Switch from `is_dirty = true` to clipping region mathematics so dragging only redraws the delta.

## What Should NEVER Be Touched
- **The PS/2 Core Initializer Sequence:** The handshaking and BAT protocols (`0xA8`, `0x20`, `0x60`, `0xD4`, etc.) are fragile and highly dependent on exact timings that work across QEMU, VirtualBox, and bare metal. Do not refactor them.
- **The `global_mouse_x` limits:** Clamping relies on engine variables linked directly to VBE.
- **Legacy Abstraction Stubs:** `BOS_GetApplication`, `BWE_GetSurface` are stubbed but must remain for ABI compatibility.

---
**Audit Complete.** 
Phase 6 Stage 2 (Implementation) is ready to commence based on these blueprints.
