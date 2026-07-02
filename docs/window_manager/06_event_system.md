# Event System

## Core Event Queue
Hardware interrupts (Keyboard/Mouse) are intercepted by `input_abstraction.c` which parses them into a unified `BVEvent` structure. 
These events are buffered into a ring buffer queue (`BWE_EventQueue`).
The main kernel loop polls `BOS_ProcessEvent()` to empty this queue.

## 1. Mouse Move & Dragging
- Mouse coordinates update `g_bwe_mouse_x` and `g_bwe_mouse_y`.
- The engine checks `bwe_drag_state`. 
- If `DRAG_DRAGGING`, it calculates the delta from the last position and calls `BOS_SetBounds()` on `bwe_drag_surface_id` to move the window.

## 2. Mouse Click
- Performs `BWE_HitTest(g_bwe_mouse_x, g_bwe_mouse_y)`.
- Reverses through the Z-Order array to find the topmost window under the cursor.
- Resolves the hit to a specific `BWE_HitZone`:
  - `BWE_HIT_TITLEBAR`: Initiates dragging lock.
  - `BWE_HIT_CLOSE`: Dispatches close event.
  - `BWE_HIT_CLIENT`: Triggers `BOS_SetFocus()` and sends `BWE_EVENT_MOUSE_DOWN` to the window's `on_event` callback.
- Controls (like Buttons) receive this event and trigger their respective user-defined `on_click` delegates.

## 3. Keyboard
- Keyboard events bypass hit-testing.
- They are routed *directly* to the `g_focused_surface_id`.
- If a Textbox control has focus, it captures alphanumeric keystrokes and appends them to its internal `text` buffer.

## Flowchart: Event Dispatch

```text
[ Hardware IRQ ] -> [ input_abstraction.c ]
                            |
                            v
                   [ BWE_EventQueue ]
                            |
                            v
                   [ BOS_ProcessEvent() ]
                            |
           +----------------+----------------+
           |                                 |
       (Mouse Event)                    (Keyboard Event)
           |                                 |
    [ BWE_HitTest() ]              [ Route to Focus ID ]
           |                                 |
   [ Z-Order Search ]               [ Window on_event() ]
           |
   [ Hit Zone Match ]
           |
  (Titlebar) -> Drag Lock
  (Client)   -> Set Focus -> [ Window on_event() ]
```
