# Cursor Pipeline

## Rendering Strategy
Cursor rendering is tightly coupled to the hardware IRQ and the Compositor (`BWE_ComposeFrame`).
Currently, cursor bounds are tracked manually. Whenever the global coordinates change (`global_mouse_x`, `global_mouse_y`), an event is dispatched.

## Redraw & Dirty Regions
- The mouse bounds do not appear to have an isolated hardware cursor or isolated rendering plane.
- The cursor is likely drawn directly onto the compositor's framebuffer output.
- **Cursor Save/Restore Logic:** The system uses standard `screen_bounds` logic or explicit compositor draw commands to composite the pointer sprite over the window hierarchy.

## Constraints
Because there is no dedicated hardware plane for the cursor, any CPU stall during `BWE_PumpEvents` or compositing directly freezes the cursor. The "mouse lag" is an artifact of mixing the synchronous event pump with the display rendering cycle.
