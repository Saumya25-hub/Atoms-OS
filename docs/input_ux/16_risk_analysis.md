# Risk Analysis

## Identified Architecture Risks

### 1. Queue Overflow (Silent Input Loss)
`event_queue` in `input.c` has `MAX_EVENTS = 64`. Modern mice output 125Hz-1000Hz. Polling at 60FPS means up to 16 packets per frame. A micro-stutter will cause `push_event` to hit `queue_tail` and drop packets silently, creating "jumpy" or unresponsive input.

### 2. Race Conditions in Event Queue
`push_event` increments `queue_head` inside the IRQ handler. `kernel_get_event` increments `queue_tail` in the main kernel thread. While integers are volatile, non-atomic increments during `cli/sti` blocks can corrupt the queue ring.

### 3. Z-Order Bubble Sort
`BWE_UpdateZOrders` uses a bubble sort mechanism. When large amounts of UI elements exist, focusing a window triggers an O(N^2) sorting stall.

### 4. Excessive Memory Bandwidth (Dragging)
Window bounds are updated synchronously during dragging. Modifying bounds sets the window and its children to dirty. The compositor immediately redraws them. Dragging requires redrawing both the old damage region (background) and the new window position, flooding the memory bus.

### 5. Infinite Hover Feedback Loops
Because `HitTest` is re-evaluated every time `MOUSE_MOVE` fires, if the mouse is placed on an edge where a control slightly alters its own size on `MOUSE_ENTER`, it will instantly exit the bounds, fire `MOUSE_LEAVE`, revert size, and re-trigger.

### 6. Focus Corruption
If `g_focused_window_id` points to a window that is unexpectedly freed without calling `BOS_DestroySurface` cleanly, the keyboard will route inputs into freed memory.
