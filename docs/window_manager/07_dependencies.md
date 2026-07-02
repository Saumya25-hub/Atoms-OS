# Dependencies

## Dependency Graph

```text
[ Desktop Shell ]
       │
       ├─> [ BWE Core ] (For Windows/Controls)
       ├─> [ BWE Paint ] (For Graphics Primitives)
       ├─> [ BOSurface ] (For direct hierarchy manipulation)
       └─> [ Memory Manager ] (For wallpaper buffer)

[ BWE Core ]
       │
       ├─> [ BOCompositor ] (To register surfaces and trigger invalidation damage)
       ├─> [ BOSurface ] (To handle actual Z-order limits and pool slots)
       └─> [ Input Abstraction ] (To consume `BVEvent` structures)

[ BOCompositor ]
       │
       └─> [ BVFramebuffer ] (To request clip constraints during rendering)
           (BOCompositor has ZERO dependencies on BWE or Shell. It is mathematically pure.)
```

## Ownership Rules
- **BOCompositor** owns the final frame compositing. It should never know about UI logic, buttons, or mouse clicks.
- **BWE** owns the state of the UI (hovered, dragged, clicked). It should not directly write to the framebuffer without going through the compositor's clip stack.
- **Desktop Shell** owns the applications and taskbar. It should never access `BOCompositor` directly; it must request repaints via `BWE_InvalidateWindow`.

## Circular Dependency Risks
Currently, `BOSurface` and `BWE` have a highly convoluted, almost circular relationship. `BWE` wraps `BOSurface` concepts, but `BOSurface` includes `bocompositor.h` and handles dragging logic that arguably belongs in `BWE`. This boundary is very weak and needs strict enforcement.
