# IRQ Pipeline Hardening

## Background
Hardware interrupts (IRQ 1 and IRQ 12) preempt the main kernel thread to deliver PS/2 bytes.

## The Threat
When an interrupt fires, the CPU saves state and jumps to the handler. If the OS reads the byte and pushes it to a ring buffer (`kbd_buffer` or `event_queue`) at the exact moment the main thread is popping from that buffer, the ring's pointers (`head` and `tail`) corrupt, leading to fatal crashes or dropped input.

## The Mitigation
Phase 6.1 enforces strict boundary control:
- `keyboard_get_event` now uses `cli` before reading `kbd_buf_tail` and `sti` after.
- `kernel_get_event` uses `cli` and `sti` around `event_queue`.

By briefly disabling interrupts during the microsecond it takes to pop an event, we guarantee the hardware handlers cannot corrupt the ring structure.
