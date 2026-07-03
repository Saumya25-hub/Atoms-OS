# Event Queue Audit

## Queue Implementation
There are three cascading queues involved in input delivery:
1. **Raw Keyboard Buffer:** `kbd_buffer[256]` in `keyboard.c`. Filled synchronously via IRQ1, emptied by polling/callbacks.
2. **BVEvent Queue:** `event_queue[64]` in `input.c`. Serves as the abstraction layer bridging raw IRQs to the UI layer. `MAX_EVENTS = 64`.
3. **BWE_Event Queue:** `g_event_queue` inside `bwe_core.c`. Fed by `BOS_ProcessEvent`.

## Synchronization and Ownership
- `push_event` in `input.c` increments `queue_head`. `kernel_get_event` increments `queue_tail`. Both use simple volatile integers.
- **Race Condition Risk:** Because interrupts (IRQs) push while the main kernel thread pops, if `cli`/`sti` are not perfectly managed around `push_event`, queue counters can corrupt.
- **Queue Overflow:** If `next_head == queue_tail`, the event is permanently lost (`bmde_state.dropped_events++`). 64 slots is dangerously small for high-poll-rate mice.

## Lifetime
Events live only as long as they are unconsumed in the ring buffer. Once popped, their data is copied by value onto the stack (`bwe_ev` in `BWE_PumpEvents`).
