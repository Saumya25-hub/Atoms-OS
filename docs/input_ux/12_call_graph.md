# Call Graphs

## Mouse Flow
```text
mouse_irq_handler (IRQ 12)
 └─ input_push_relative (input_abstraction layer)
     └─ kernel_input_push_mouse_absolute
         └─ push_event (adds BVEvent)
```

## Pulse / Pump Flow (Every Frame)
```text
BOHeart_Pulse
 ├─ BWE_PumpEvents
 │   ├─ BWE_EventQueue_Pop
 │   ├─ BWE_ProcessMouseInteraction (Updates dragging)
 │   │   └─ BOS_SetBounds -> BWE_InvalidateWindow
 │   ├─ BWE_HitTest (Resolves hover/click targets)
 │   ├─ BOS_SetFocus (If clicked)
 │   │   └─ BWE_BringToFront -> BWE_UpdateZOrders (Bubble sort)
 │   └─ window->on_event() (Dispatches payload to app)
 └─ BWE_ComposeFrame
     └─ Rebuilds Framebuffer from Dirty Windows
```

## Keyboard Flow
```text
keyboard_irq_handler (IRQ 1)
 └─ active_driver->read_scancode()
 └─ Converts to ASCII
 └─ key_callback (kernel_input_push_key_event)
     └─ push_event (adds BVEvent)
```

## Focus Switching
```text
BOS_SetFocus(target)
 ├─ BWE_GetWindow(old)
 ├─ old->state = DEACTIVATED
 ├─ BWE_InvalidateWindow(old)
 ├─ target->state = ACTIVE
 ├─ BWE_InvalidateWindow(target)
 └─ BWE_BringToFront(target)
     └─ BWE_UpdateZOrders()
```
