# Input & UX Architecture Overview

## Mission Summary
This document serves as the high-level summary of the entire Input and User Experience (UX) Architecture in ATOMS OS Foundation v2 (Phase 6).

## Scope
The Input/UX subsystem spans across:
1. **Hardware Drivers:** PS/2 Controller, Mouse, and Keyboard IRQ Handlers.
2. **Input Abstraction Layer:** `kernel/drivers/input/input.c` that aggregates hardware events into standard `BVEvent` structures.
3. **Window Engine (BWE):** The core windowing and compositing engine (`bwe_core.c`, `bwe_window.c`) which is responsible for hitting testing, event routing, dragging, and focus management.
4. **Desktop Shell:** Uses the primitives exposed by BWE to manage user interactions like Start Menu, Task Panel, and icons.

## Key Subsystems
- **Mouse & Keyboard Pipeline:** From IRQs (12 and 1) down to the `BWE_Event` queue, events are asynchronously buffered and pumped synchronously during the `BOHeart_Pulse`.
- **Event Queue:** A ring-buffer design buffering hardware events before the Window Manager consumes them.
- **Hit Testing:** O(N) evaluation of Window bounds across a Z-sorted stack.
- **Focus & Z-Order Management:** Global focus tracking, managing active and top-most states, backed by a dynamic (and currently inefficient) bubble-sort algorithm.

## State of the Architecture
The architecture is functional but suffers from massive synchronous overheads, tight coupling, and O(N^2) algorithms in hot paths. The primary issues stem from naive hit-testing, lack of a true dirty-region clipping system, and global event dispatch that blocks rendering.

> **Read Next:** proceed to `02_mouse_architecture.md` and `03_keyboard_architecture.md` for low-level details.
