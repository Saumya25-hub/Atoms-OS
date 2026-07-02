# ATOMS OS Foundation v2 Manifest

## Purpose
The Foundation v2 Manifesto defines the philosophical and technical direction of ATOMS OS. It acts as the anchor for all future development, ensuring the OS remains minimal, fast, and modular.

## Responsibilities
- Define the core UI and architectural boundaries.
- Outline the guiding principles of the "AntiGravity Development Mission".
- Establish the debugging and maintainability standards.

## Architecture
The Foundation v2 architecture is divided into clear layers:
1.  **Core Kernel:** Scheduling, Memory, Interrupts.
2.  **Horse Engine:** Central execution engine, fast lookups, and resource management.
3.  **VFS & Storage:** File system abstraction.
4.  **UI & Graphics:** Minimal Compositor and Window Manager (no blur, transparency).
5.  **Userspace:** Essential applications (Terminal, Notes, Files, Search).

## Flow
1.  **Boot:** Initialize basic hardware and memory.
2.  **Kernel Init:** Setup interrupts, scheduling, and VFS.
3.  **Engine Init:** Start the Horse Engine for high-speed indexing and app launching.
4.  **UI Init:** Launch the minimal compositor and window manager.
5.  **Shell:** Present the floating Task Panel and Start Menu to the user.

## Future Expansion
-   Transition to a more sophisticated cache system within the Horse Engine.
-   Introduce polished UI features (glass effects, rounded corners) *only* after stability and performance are proven.

## TODO
- [ ] Migrate all existing code into the `kernel/core` and `kernel/ui` structural paradigm.
- [ ] Implement the first iteration of the Horse Engine.
