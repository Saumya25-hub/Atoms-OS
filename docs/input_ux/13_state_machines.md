# State Machines

## Mouse State
While implicit, the mouse hardware effectively flows through:
```text
Idle → (IRQ byte 1) → Partial X → (IRQ byte 2) → Partial Y → (IRQ byte 3) → Complete Packet → Dispatch → Idle
```

## Window Dragging / Resizing State Machine
Controlled in `BWE_ProcessMouseInteraction`.

```text
[Idle]
   ↓ (MOUSE_DOWN && Hit=Titlebar)
[Dragging Capture]
   ↓ (MOUSE_MOVE)
[Moving / Invalidating Bounds]
   ↓ (MOUSE_UP)
[Release / Idle]
```

Same applies for Resizing, except the modifier calculates 8-way delta offsets depending on `s_resize_zone`.

## Focus State Machine
```text
[DEACTIVATED]
   ↓ (BOS_SetFocus)
[ACTIVE & TOPMOST]
   ↓ (Another window clicked)
[DEACTIVATED]
   ↓ (BOS_Hide)
[HIDDEN]
```

## Cursor Hover
```text
[Idle / Outside]
   ↓ (Mouse Move intersects Bounds)
[MOUSE_ENTER Dispatched]
   ↓ (Mouse remains inside Bounds)
[Hovering]
   ↓ (Mouse leaves Bounds)
[MOUSE_LEAVE Dispatched]
```
The hover state resolves leaf controls dynamically and stores `s_hovered_control_id`.
