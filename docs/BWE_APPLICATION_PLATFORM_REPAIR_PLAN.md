# BWE APPLICATION PLATFORM REPAIR PLAN

## Overview
This document outlines the systematic implementation plan to fix the architectural layout, clipping, and resizing flaws in the ATOMS OS Window Engine (BWE) and Application layer, in accordance with the `SIGNATURES ATOMS OS — APPLICATION PLATFORM REPAIR` directive.

## User Review Required
> [!IMPORTANT]
> The Terminal's real shell command dispatcher currently resides in `userspace/shell/command.c`. We need to decide whether to statically link these userspace shell components into the kernel space for the GUI terminal to use directly, or if we should define a new kernel-mode IPC mechanism so the GUI terminal can spawn the userspace shell process and pipe its I/O. The current plan assumes we can link and call the `command_execute()` functions directly or extract a shared core, but user confirmation on the preferred kernel/user boundary is requested.

## Proposed Changes

### Phase 1 & 2: Geometry Contract & Guard
Define a strict geometry contract distinguishing absolute frame bounds from client bounds.
#### [NEW] `kernel/wm/bwe/include/bwe_geometry.h`
#### [NEW] `kernel/wm/bwe/src/bwe_geometry.c`
#### [NEW] `kernel/wm/bwe/include/bwe_diagnostics.h`
#### [NEW] `kernel/wm/bwe/src/bwe_diagnostics.c`
#### [MODIFY] `kernel/wm/bwe/src/bwe_window.c`
- **Goal:** Update `BOS_CreateSurface` and layout logic to calculate child `screen_bounds` relative to the parent's `client_bounds` instead of `frame_bounds`. Introduce functions like `BWE_GetClientBounds()` and integrate runtime geometry validation via `bwe_diagnostics.c`.

### Phase 3 & 4: Unified Render Context & Clipping Fix
Fix the P0 bug where `BOImage` and `BOFont` bypass BWE clipping.
#### [NEW] `kernel/wm/bwe/include/bwe_render_context.h`
#### [NEW] `kernel/wm/bwe/src/bwe_render_context.c`
#### [MODIFY] `kernel/ui/boimage/boimage.c`
#### [MODIFY] `kernel/ui/bofont/bofont.c`
#### [MODIFY] `kernel/wm/bwe/renderer/bwe_compositor.c`
- **Goal:** Define a `BWE_RenderContext` representing the current clipping state. Add context-aware entry points (e.g., `BOImage_AtlasDrawExCtx`, `BOFont_DrawTextCtx`) that enforce intersection with the active context clip before falling back to raw `BOVISUAL_Graphics_PutPixel()`. Update `BWE_ComposeFrame` to manage and push these contexts.

### Phase 5 & 6: Adaptive Layout & Maximize/Restore
Support dynamic window resizing without breaking application layout.
#### [NEW] `kernel/wm/bwe/include/bwe_layout.h`
#### [NEW] `kernel/wm/bwe/src/bwe_layout.c`
#### [MODIFY] `kernel/wm/bwe/include/bwe.h`
#### [MODIFY] `kernel/wm/bwe/src/bwe_window.c`
- **Goal:** Introduce simple layout anchors (TOP, BOTTOM, LEFT, RIGHT, FILL). Add `restore_bounds` to `BWE_Window`. Implement `BWE_MaximizeWindow` and `BWE_RestoreWindow` that re-evaluate the layout pass upon resize, clamping to min/max dimensions. 

### Phase 7 & 8: Hit Testing & Application Migration
Synchronize hit testing with actual rendering and update hardcoded applications.
#### [MODIFY] `kernel/wm/bwe/src/bwe_window.c`
#### [MODIFY] `kernel/shell/desktop_shell/apps.c`
- **Goal:** Ensure `BWE_HitTest` evaluates the resolved effective clip so invisible overflowing controls are untargetable. Refactor `apps.c` applications (Settings, Calculator, Terminal, Music Player) to position their controls relative to (0,0) of the new client area (removing manual +30 titlebar offsets) and attach layout anchors. Replace stale `1280x720` references with capabilities queries.

### Phase 9: Terminal Real Shell Integration
#### [MODIFY] `kernel/shell/desktop_shell/apps.c`
- **Goal:** Remove the fake `strcmp` command handler in `terminal_textbox_event_callback`. Wire up the input submission to a proper command dispatcher (e.g., `command_execute` from the registry), sharing logic with the console shell.

### Phase 10 & 11 & 12: Diagnostics & Developer Tools
#### [MODIFY] `kernel/shell/desktop_shell/apps.c`
#### [MODIFY] `kernel/wm/bwe/renderer/bwe_compositor.c`
- **Goal:** Add a "Window Diagnostics" tab in the Settings app that reads the ring buffer in `bwe_diagnostics.c`. Add a toggle for a visual debug overlay that outlines frame bounds, client bounds, and clips at the end of the compositor pass.

### Phase 13: Window Contract Self-Test
#### [NEW] `kernel/wm/bwe/src/bwe_test.c`
#### [MODIFY] `kernel/shell/desktop_shell/apps.c`
- **Goal:** Add a "Window Contract Test" button/application that automatically spawns windows, places overlapping controls, resizes them, and maximizes them, asserting zero geometry/clipping violations in the diagnostics engine.

## Verification Plan
### Automated Tests
- Run the newly created BWE Window Contract Self-Test.
- Check the geometry diagnostics ring buffer for any unhandled layout violations or overlapping controls during resize operations.

### Manual Verification
- Spawn Settings, Terminal, and Calculator.
- Resize each window manually and confirm that controls adapt dynamically (anchoring/filling correctly).
- Verify that scrolling in Settings clips the text and sprites at the exact client boundaries.
- Execute commands in the GUI terminal and confirm real shell responses.
- Open Settings > Developer > Window Diagnostics and verify the live telemetry is accurate.
