# Global Variables Audit

The Input and UX system heavily relies on global state.

## Mouse (drivers/input/ps2/mouse.c)
- `mouse_cycle` (uint8_t): Tracks which of the 3 bytes is being read.
- `mouse_byte[3]` (uint8_t[]): Packet buffer.
- `last_byte_time` (uint64_t): Timeout watchdog.
- `diag` (PS2MouseDiagnostics): Telemetry.

## Input Abstraction (kernel/drivers/input/input.c)
- `event_queue[64]` (BVEvent[]): Abstraction queue.
- `queue_head`, `queue_tail` (volatile int): Ring buffer pointers.
- `global_mouse_x`, `global_mouse_y` (int32_t): Screen coordinates.
- `global_mouse_buttons` (uint8_t): Button mask.

## Keyboard (kernel/drivers/keyboard/src/keyboard.c)
- `active_driver` (KeyboardDriver*): Abstraction layer.
- `shift_pressed`, `ctrl_pressed`, `alt_pressed`, `caps_lock_on` (bool): Modifier state.
- `kbd_buffer[256]` (KeyboardEvent[]): Raw queue.
- `kbd_buf_head`, `kbd_buf_tail` (volatile uint32_t).

## Window Manager Core (kernel/wm/bwe/src/bwe_core.c)
- `g_windows[BWE_MAX_WINDOWS]` (BWE_Window[]): Master pool.
- `g_event_queue` (BWE_EventQueue): App-facing queue.
- `g_active_window_id`, `g_focused_window_id` (uint32_t): Focus tracking.
- `g_z_order_stack[BWE_MAX_WINDOWS]`, `g_z_stack_count`: Global Z-sorting layer.
- `g_bwe_mouse_x`, `g_bwe_mouse_y`: BWE's local clone of coordinates.

## Window Drag/Resize State (kernel/wm/bwe/src/bwe_window.c)
- `s_is_dragging`, `s_is_resizing` (bool)
- `s_drag_win_id`, `s_resize_win_id` (uint32_t)
- `s_drag_offset_x`, `s_drag_offset_y`, `s_resize_start_x`, `s_resize_start_y` (int32_t)
- `s_resize_zone` (BWE_HitZone)
- `s_prev_buttons` (uint8_t): Edge trigger tracker.
