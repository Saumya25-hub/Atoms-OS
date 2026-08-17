# FORENSIC REPORT — MILESTONE 2: BWE COMPOSITOR & INTERACTIVE RING 3 GUI

## 1. Root Cause & Gap Analysis
In Option A, the Ring 3 user process (`gui_demo`) was proven to execute at CPL=3, successfully issuing GUI syscalls (`CREATE_WINDOW`, `MAP_SURFACE`, `SHOW_WINDOW`, `POLL_EVENT`). 

However, to complete Milestone 2 (visual compositing and live mouse/keyboard interaction), the following architectural gaps exist between Ring 3 private memory and the Ring 0 BWE Compositor:

1. **Surface Buffer Storage & Mapping Discrepancy**:
   - `sys_service_gui_map_surface` previously allocated separate non-contiguous physical pages via `vmm_map_user_page` and pointed `win->control_data.canvas.pixel_buffer` to physical page 0.
   - When the Ring 0 BWE Compositor attempts to blit `win->control_data.canvas.pixel_buffer`, it requires a kernel virtual address that maps the entire surface linearly.
   - **Resolution**: Allocate a kernel-linear buffer via `kmalloc_aligned`, and map those physical pages directly into `cur->pml4` at user virtual address `0x50000000 + win_id * 0x1000000` with `PAGE_USER | PAGE_WRITABLE | PAGE_PRESENT`. This guarantees zero-copy coherency: Ring 3 writes to `0x50000000`, and Ring 0 BWE Compositor immediately sees those exact pixels at its kernel virtual buffer.

2. **Immediate Damage Notification on Invalidate / Show**:
   - When Ring 3 calls `SYS_GUI_INVALIDATE` or `SYS_GUI_SHOW_WINDOW`, `BWE_InvalidateWindow(win_id)` marks the window dirty, but `BWE_Compose()` should be immediately triggered or serviced by the heart pulse so the frame is presented to the hardware framebuffer.

3. **Input Core Event Routing to Ring 3 Queue**:
   - `BWE_PumpEvents` already hit-tests windows and has `sys_gui_post_event(leaf_id, &gui_ev)`.
   - In `sys_gui_post_event`, ensure window ID and event types (Move, Down, Up, Key) are preserved and deliverable to `SYS_GUI_POLL_EVENT`.
   - Also, when window titlebar is clicked and dragged, BWE's window dragging updates `win->screen_bounds`, and a `BOS_GUI_EVENT_WINDOW_MOVED` / drag confirmation is posted to Ring 3.

4. **Deterministic Forensic Markers**:
   - Insert deterministic markers:
     - `[RING3_GUI] CREATE PASS`
     - `[RING3_GUI] SURFACE MAP PASS`
     - `[RING3_GUI] DRAW PASS`
     - `[RING3_GUI] INVALIDATE PASS`
     - `[BWE_GUI] SURFACE COMPOSITE PASS`
     - `[BWE_GUI] HITTEST PASS`
     - `[BWE_GUI] EVENT ROUTE PASS`
     - `[RING3_GUI] MOUSE EVENT RECEIVED`
     - `[RING3_GUI] KEY EVENT RECEIVED`
     - `[RING3_GUI] DRAG PASS`

## 2. Files Involved
- `kernel/core/syscall/src/services.c`
- `kernel/wm/bwe/renderer/bwe_compositor.c`
- `kernel/wm/bwe/src/bwe_core.c`
- `kernel/kernel.c`
- `userspace/apps/gui_demo/main.c`

## 3. Risk Analysis
- **Low Risk**: No architectural changes to kernel scheduling, ROOK, xHCI, or desktop shell.
- **Security Invariant**: Ring 3 never receives framebuffer physical/virtual addresses; only accesses its own isolated `0x50000000` page range.
