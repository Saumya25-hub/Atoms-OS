# UI & Compositor Architecture

## Purpose
Define the minimalist display server, compositor, and window manager for ATOMS OS Foundation v2. The focus is exclusively on speed, predictable rendering, and professional simplicity.

## Responsibilities
- **Compositor:** Mixes window buffers into a final screen output. Absolutely no blur, transparency, or glass effects. No rounded corners.
- **Window Manager:** Handles the placement of windows, focus states, and input routing (mouse/keyboard).
- **Task Panel:** Bottom-center floating panel containing ATOMS, Search, Files, Notes, Terminal, and Time/Date. No widgets.
- **Start Menu:** Pinned apps (Search, Files, Notes, Terminal) and a right panel (ATOMS Profile, Settings, Restart, Power Off).

## Architecture
- **BOSurface / bocompositor based:** Built on our internal graphics primitives.
- **Modular Shell:** The Task Panel and Start Menu are independent components that communicate with the Window Manager.
- **Input Routing:** A clean event-driven model passing keyboard and mouse events to the focused window.

## Flow
1. Graphics driver initializes the framebuffer.
2. Compositor starts and requests the desktop background buffer.
3. Shell (Task Panel / Start Menu) is instantiated and registered as a privileged system window.
4. When a user clicks, the Window Manager calculates the hit-test and routes the event.

## Future Expansion
- After Foundation v2 achieves total stability and speed, we will selectively introduce polish (glass effects, rounded corners) as optional UI features.

## TODO
- [ ] Strip out any existing code related to visual effects (transparency, rounded corners).
- [ ] Implement the floating bottom-center Task Panel.
- [ ] Implement the minimal Start Menu layout.
