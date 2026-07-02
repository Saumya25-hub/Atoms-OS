# Window Manager Overview

## What is the Window Manager?
In the current (legacy) architecture of ATOMS OS, the "Window Manager" is not a single isolated module, but a heavily intertwined collection of subsystems spanning `BOSurface`, `BWE` (BOSurface Window Environment), `BOCompositor`, and `Shell`. It is responsible for orchestrating the lifecycle, positioning, and rendering of all graphical elements on the screen.

## Responsibilities
1. **Surface Management (`BOSurface`)**: Maintains a static pool of surfaces (max 128/1024 depending on the subsystem limit), managing parent-child hierarchies.
2. **Window Abstraction (`BWE`)**: Provides the `BWE_Window` control structure, handling everything from bounds (geometry), styles, flags (resizable, movable, borderless), and unified control rendering states (e.g., button pressed, textbox cursor).
3. **Compositing (`BOCompositor`)**: Tracks screen damage (dirty rectangles), manages Z-order occlusion, and calculates clipping regions to execute a zero-allocation, back-to-front render pass.
4. **Input Routing**: Captures mouse and keyboard events and routes them to the focused or hovered window via hit testing.
5. **Desktop Shell (`Shell`)**: Renders the bottom taskbar, the background wallpaper, and the legacy start menu.

## What does it NOT control?
- **Low-Level Hardware**: It does not speak directly to the GPU/VGA. It relies on a generic `BVFramebuffer` (BOS Visual Framebuffer) provided by the external graphics driver.
- **Process Memory Isolation**: The Window Manager tracks an `owner_pid` for surfaces, but it relies entirely on the kernel's scheduler and VMM for actual memory protection.
- **Hardware Interrupts**: It does not hook PS/2 interrupts directly; it receives generic event structs (`BVEvent`) parsed by an external input abstraction layer.
