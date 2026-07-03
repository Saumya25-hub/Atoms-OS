# Dependency Graph

## Top-to-Bottom Flow

```mermaid
graph TD
    A[Hardware PS/2 IRQs] -->|Byte Streams| B[ps2_mouse / ps2_keyboard drivers]
    B -->|Relative DX/DY, Scancodes| C[input.c Abstraction]
    C -->|BVEvent Queue| D[BOS_ProcessEvent / bwe_core.c]
    D -->|BWE_Event Queue| E[BWE_PumpEvents]
    E -->|BWE_HitTest| F[Z-Order Stack]
    E -->|Focus State| G[Keyboard Routing]
    E -->|BWE_ProcessMouseInteraction| H[Drag/Resize State Machine]
    F -->|Target Resolution| I[Leaf Window / Control]
    H -->|BOS_SetBounds| J[BWE_InvalidateWindow]
    I -->|on_event| K[Application Level]
    J -->|Damage| L[Compositor Framebuffer]
```

## Circular Dependencies & Coupling
- **Input & Display:** The `input.c` layer relies directly on `g_kernel_screen_width` and `g_kernel_screen_height` from the VBE driver to clamp mouse coordinates.
- **BWE Core & BWE Window:** `bwe_core.c` calls `BWE_ProcessMouseInteraction` (in `bwe_window.c`), which accesses the Z-order globals defined in `bwe_core.c`.
- **Keyboard & Input:** `keyboard.c` uses a callback to `input.c` which relies on global mouse variables (`global_mouse_x`, etc.) to perfectly pack a `BVEvent` with both keyboard and mouse state.
