# Mouse Architecture Pipeline

## The Complete Path
The mouse pipeline maps raw electrical signals to application-level input events.

```text
PS/2 IRQ 12
↓
Interrupt Handler (ps2_mouse_wait / ps2_mouse_read)
↓
Input Driver (ps2_mouse_init, mouse_irq_handler in mouse.c)
↓
Mouse Decoder (Extracts DX, DY, Buttons with 1:1 unscaled translation)
↓
input_push_relative -> kernel_input_push_mouse_absolute
↓
Input Queue (kernel/drivers/input/input.c, BVEvent array)
↓
Event Queue Translation (BOS_ProcessEvent translates BVEvent to BWE_Event)
↓
Event Pump (BWE_PumpEvents pops events from g_event_queue)
↓
Window Manager (bwe_core.c)
↓
Hit Testing (BWE_HitTest across Z-order)
↓
Focus / Hover Routing (Leaf-most child resolution)
↓
Dragging / Resizing (BWE_ProcessMouseInteraction)
↓
Cursor Update / Compositor (Via BOS_SetBounds or internal BWE updates)
↓
Framebuffer
```

## Global Variables & State
- `mouse_byte[3]`: Ring buffer storing the current packet.
- `last_byte_time`: Synchronization timestamp to prevent packet desync.
- `global_mouse_x`, `global_mouse_y`: Screen coordinates managed by the kernel input system.
- `global_mouse_buttons`: Bitmask of active buttons.
- `s_prev_buttons`: Used in `bwe_window.c` to compute edge triggers for MOUSE_DOWN and MOUSE_UP.

## Failures and Deficiencies
1. **Packet Desynchronization:** If an IRQ fires but the byte is missed, the driver relies on a timeout `(current_time - last_byte_time) > 10` to reset the `mouse_cycle`.
2. **No Hardware Batching:** Polling is byte-by-byte (`io_in8`), causing 3 interrupts per mouse move, starving the CPU during fast motions.
3. **Queue Overflow:** If `queue_head` hits `queue_tail` inside `push_event`, mouse events are dropped silently.
4. **Latency:** Dispatch requires a full O(N) deep-tree hit-test on every `BWE_EVENT_MOUSE_MOVE` which delays the cursor redraw.
