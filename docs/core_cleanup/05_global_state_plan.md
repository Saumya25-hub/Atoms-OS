# Global State Plan

## Problem
The UI subsystem relies heavily on scattered, unprotected global variables (`g_bwe_mouse_x`, `bwe_is_dragging`, `g_focused_window_id`, `g_update_lock`). This creates an incredibly brittle environment, prone to race conditions and difficult to debug.

## Cause
Lack of a structured Window Manager context. Data was added globally whenever a new feature (like dragging or hover states) was needed.

## Solution
We will consolidate these scattered globals into a single, cohesive context structure.

```c
typedef struct {
    // Focus Subsystem
    uint32_t focused_surface_id;
    uint32_t active_surface_id;

    // Mouse Tracking Subsystem
    int32_t  mouse_x;
    int32_t  mouse_y;
    uint32_t hover_surface_id;
    uint32_t capture_surface_id;

    // Drag Subsystem
    DragState drag_state;
    uint32_t  drag_surface_id;
    int32_t   drag_offset_x;
    int32_t   drag_offset_y;

    // Locks
    uint8_t   update_lock;
} WMContext;

extern WMContext g_wm_context;
```
*Note: We are not eliminating the global instance `g_wm_context` in this phase (as per the user directive), but we are grouping the loose variables into this controlled struct.*

## Risk
Modifying every reference to these variables (`bwe_is_dragging` -> `g_wm_context.drag_state`) touches nearly every file in the UI subsystem. A typo will break mouse clicks or rendering.

## Verification
The behavior must remain identical. Dragging windows and clicking buttons should work exactly as they did before the contextualization.

## Next Step
Define the strict module boundaries to prevent these subsystems from bleeding into each other again (`06_module_boundaries.md`).
