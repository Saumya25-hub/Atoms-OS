# Window Manager Workflows

## Desktop Boot Sequence

```text
Kernel Boot
    ↓
VBE Graphics Init (Framebuffer established)
    ↓
BOSurface_Init() (Zeroes out the surface pool)
    ↓
BOCompositor_Initialize() (Clears Damage, Stack, Clips)
    ↓
BWE_Initialize() (Sets up Event Queue)
    ↓
Shell Init / Apps Registration (Terminal, Settings registered)
    ↓
Taskbar Created (Dock panel, Start menu created but hidden)
    ↓
Main Kernel Loop Begins
```

## Render Loop Workflow

The Window Manager does not render continuously. It renders strictly on demand when Damage (Dirty Rectangles) exist.

```text
Event Occurs (e.g., Mouse Move)
    ↓
BWE_InvalidateWindow() called (Marks window dirty)
    ↓
BOCompositor_AddDamage() (Adds bounding box to damage array)
    ↓
Main Loop Checks BOCompositor_HasDamage()
    ↓
BOCompositor_ComposeFrame() executes
    ↓
    ├── Pass 1: Occlusion Calculation (Finds hidden windows)
    ↓
    ├── Pass 2: Back-to-Front Render Execution
    ↓
    └── Triggers BWE/Shell `on_render` callbacks with specific clip rects
    ↓
Frame Complete
    ↓
BOCompositor_ClearDamage()
```

## Input Event Workflow (Mouse Click)

```text
Mouse Driver interrupt triggers
    ↓
input_abstraction pushes BVEvent to BWE_EventQueue
    ↓
BOS_ProcessEvent() consumes queue
    ↓
Calculates Hit Test via BWE_HitTest()
    ↓
    ├── If hits Titlebar -> Begins dragging state (`bwe_drag_surface_id`)
    ├── If hits Close Button -> Triggers BOS_DestroySurface()
    └── If hits Client Area -> Sets Focus (`BOCompositor_SetFocus`)
    ↓
Routes BWE_Event to the specific window's `on_event` callback
```
