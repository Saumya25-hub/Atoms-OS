# Implementation Plan: Virtual Terminal (VT) Architecture

## Goal Description
Implement a commercial-grade Virtual Terminal (VT) architecture to correctly multiplex the graphical framebuffer between the Boot/Kernel Console (VT1) and the Desktop GUI (VT2). This resolves the issue of diagnostic text blindly overwriting the desktop graphics by formally decoupling the rendering layers and assigning framebuffer ownership based on the active VT.

## User Review Required
> [!IMPORTANT]
> The current system has no text buffer for the console; text is drawn directly to VRAM. To correctly support switching *back* to VT1 from VT2, I propose adding a lightweight text-mode shadow buffer (e.g., 80x25 or equivalent characters) in `vt_console.c` so the text can be correctly restored when the user presses `Ctrl+Alt+F1`. Please confirm if this is acceptable.

## Open Questions
- Is `VT3` (Live Debug Console) meant to be implemented now, or just scaffolded as an enum/stub for the future? (I will scaffold it for now).
- Is there a preferred location for `vt_manager.init()` inside `kernel.c`? (I will place it right after `console_set_backend()` to take over the console immediately).

## Proposed Changes

---

### Terminal Subsystem Layer
This new subsystem acts as the single authority over the framebuffer.

#### [NEW] `kernel/terminal/vt_manager.h`
- Defines `VT_Type` enum (`VT_CONSOLE=1`, `VT_DESKTOP=2`, `VT_DEBUG=3`).
- Declares `vt_manager_init()`, `vt_manager_switch(VT_Type target)`, `vt_manager_get_active()`.

#### [NEW] `kernel/terminal/vt_manager.c`
- Manages the state of `active_vt`.
- Dispatches ownership switches to `vt_console_activate()` or `vt_gui_activate()`.

#### [NEW] `kernel/terminal/vt_console.c`
- Implements VT1.
- Intercepts `console_draw_char`. 
- Maintains a text buffer to allow redrawing the console when switching back to VT1.
- If `active_vt == VT_CONSOLE`, it passes the draw command to the underlying VBE console backend.
- If `active_vt != VT_CONSOLE`, it only stores the text in the buffer (serial is handled independently).

#### [NEW] `kernel/terminal/vt_gui.c`
- Implements VT2.
- Exposes `vt_gui_activate()`, which injects a full-screen dirty rect to the BWE Compositor to force a full desktop redraw when switching back from VT1.

#### [NEW] `kernel/terminal/vt_input.c`
- Implements keyboard shortcut interception (`Ctrl+Alt+F1`, `Ctrl+Alt+F2`).
- Provides a hook to the Input Dispatcher.

---

### Input Dispatcher Layer
Integrates the VT shortcut interception at the earliest input stage.

#### [MODIFY] `kernel/drivers/input/dispatcher/dispatcher_filters.c`
- Inject `vt_input_process(event)` before other filters.
- If the event is `Ctrl+Alt+F1/F2`, the filter consumes it (returns false to drop it from normal routing) and triggers `vt_manager_switch()`.

---

### Console Layer
Integrates the console backend with the VT manager.

#### [MODIFY] `kernel/shell/console/console.c`
- Update `console_draw_char()` and `console_clear_all()` to route through `vt_console.c` instead of directly hitting the active backend.
- This ensures that only VT1 can draw to the screen.

---

### Kernel Boot Layer

#### [MODIFY] `kernel/kernel.c`
- Add `vt_manager_init()` after VBE initialization.
- Add `vt_manager_switch(VT_DESKTOP)` inside or right before `Desktop_Shell_Initialize()` / main loop entry.

---

### Documentation

#### [NEW] `docs/architecture/virtual_terminal_architecture.md`
- Create the requested architectural documentation, including diagrams, boot flow, and ownership flow.

## Verification Plan

### Automated Tests
- Build verification to ensure all new modules compile and link successfully.

### Manual Verification
1. Boot the OS.
2. Verify boot logs appear on VT1.
3. Verify the transition to VT2 (Desktop) is clean and automatic.
4. Verify moving the mouse no longer prints `[INPUT TRACE] Queue Pop` over the GUI.
5. Press `Ctrl+Alt+F1` -> Verify the screen instantly switches to the text console and recent logs are visible.
6. Press `Ctrl+Alt+F2` -> Verify the screen instantly switches back to the Desktop with a clean redraw.
