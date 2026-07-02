# Complete Window Manager Architecture

## Subsystem Tree

```text
Window Manager Ecosystem
│
├── BWE (BOSurface Window Environment) -> `kernel/bwe`
│   ├── BWE Core: Window Registry (Static Array), Geometries, Invalidation Queue
│   ├── BWE Controls: Specific UI Widgets (Button, Textbox, Canvas)
│   ├── BWE Paint: Drawing primitives (Lines, Rects, Text)
│   ├── BWE Events: Event Queue for input routing
│   └── BWE Theme: Hardcoded colors and styles
│
├── BOSurface -> `kernel/BOSurface/Core`
│   └── Surface Manager: Low-level parent-child tree, bounds calculation
│
├── BOCompositor -> `kernel/bocompositor`
│   ├── Compositor Stack: Z-Order sorting
│   ├── Compositor Damage: Dirty rectangle tracking
│   ├── Compositor Clip: Screen clipping bounds
│   └── Compositor Core: Back-to-front rendering loop with Occlusion Culling
│
└── Desktop Shell -> `kernel/shell`
    ├── Desktop: Wallpaper rendering
    ├── Taskbar: Dock panel, Clock, Window tracking
    ├── App Registry: Shell_RegisterApp logic
    └── Legacy Start Menu: Procedural menu for launching apps
```

## Detailed Explanations

### BWE (BOSurface Window Environment)
The brain of the UI. `BWE_Window` acts as an OOP base class (represented via a massive C `union` for controls). It tracks absolute and local bounds, margins, padding, and handles Hit Testing (`BWE_HitTest`).

### BOSurface
A lower-level module (historically the predecessor to BWE, now intertwined). It handles tree transversals to find top-level windows and tracks global drag states (`bwe_is_dragging`, `bwe_drag_surface_id`).

### BOCompositor
The rendering backend. It abstracts away `BWE_Window` into a simpler `BOCompositorSurface` containing only geometry and Z-order. It is optimized for zero-allocations during the render loop. It uses an occlusion pass to skip rendering fully covered windows.

### Desktop Shell
A highly privileged "Userspace" application running in ring 0. It binds directly to BWE hooks (`on_render`, `on_event`) to draw the taskbar (`taskbar.c`) and manages the telemetry/diagnostics HUD.

## Dependencies & Relationships
- **Shell** depends on **BWE** and **BOCompositor**.
- **BWE** depends heavily on **BOCompositor** (for `BOCompositor_Invalidate`) and **BOSurface** (for hierarchy).
- **BOCompositor** is completely decoupled from UI logic; it only understands rectangles and IDs. This is the cleanest boundary in the current architecture.
