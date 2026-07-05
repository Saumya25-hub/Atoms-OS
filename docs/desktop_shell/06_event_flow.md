# Event Routing

ATOMS OS Phase 8.3 establishes a strict Z-order event routing priority to prevent windows from stealing clicks intended for the shell.

## The Flow

Inside `interaction_engine_update()`:

1. **Mouse Input Poll**: Raw `InputState` changes are detected.
2. **Shell Hit Test**: `desktop_shell_hit_test()` is called first.
   - If the cursor is over the Taskbar, the Taskbar consumes the event.
   - If the cursor is over an Icon, the Icon updates its state.
3. **Window Hit Test**: If the shell ignores the event (e.g. it fell on the empty desktop, or over a window), `window_get_at_point()` determines which Application Window was clicked.
4. **Event Dispatch**: The event is finally dispatched to the respective Window's `GUIEventQueue`.
