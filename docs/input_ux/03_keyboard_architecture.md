# Keyboard Architecture Pipeline

## The Complete Path

```text
PS/2 IRQ 1
↓
Interrupt Handler (keyboard_irq_handler in keyboard.c)
↓
Scancode Reading (from active_driver->read_scancode)
↓
Translation (Scancode to ASCII mapping via scancode_to_ascii / scancode_to_ascii_shift)
↓
Input Queue (kbd_buffer[256], and pushed to kernel_input_push_key_event)
↓
Event Queue Translation (BVEvent -> BWE_Event)
↓
Focused Window Resolution (BWE_PumpEvents routing to g_focused_window_id)
↓
Control Delivery (target->on_event)
↓
Application
```

## Internal Mechanics
- **Modifier State:** Tracked via global booleans: `shift_pressed`, `ctrl_pressed`, `alt_pressed`, `caps_lock_on`.
- **Extended Keys:** `0xE0` prefixes trigger the `expect_e0` boolean, mapping raw scancodes to virtual keys like `BOS_KEY_UP`, `BOS_KEY_HOME`, etc.
- **ASCII Conversion:** Naive table lookup `scancode_to_ascii` with XOR logic for Caps Lock and Shift mapping.

## Dependencies and Callback Chain
- `keyboard.c` exposes `keyboard_register_callback`.
- `input.c` hooks this via `kernel_input_push_key_event`.
- The event is formatted as `BV_EVENT_KEY_DOWN` / `UP` and sent into the `BVEvent` pipeline.

## Limitations and Risks
1. **Localization:** Hardcoded to US QWERTY Set 1. There is no concept of a keyboard layout registry or dead keys.
2. **Key Repeat / Rate:** There is no software tracking of key repeat timing; it relies completely on hardware auto-repeat which can overwhelm the queue.
3. **Queue Overflow:** Fixed 256-event buffer `kbd_buffer`.
4. **Global Shortcuts:** The dispatch loop does not intercept global OS shortcuts (like Alt+Tab) cleanly before routing to the application.
