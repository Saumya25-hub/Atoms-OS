# PATCH PLAN — MILESTONE 2: BWE COMPOSITOR & INTERACTIVE RING 3 GUI

## 1. Objectives
Implement the complete end-to-end pipeline:
`RING 3 gui_demo` ➔ `SYS_GUI_CREATE_WINDOW` ➔ `SYS_GUI_MAP_SURFACE` ➔ User Pixel Painting ➔ `SYS_GUI_INVALIDATE` ➔ `Ring 0 BWE Compositor` ➔ Window Client-Area Blit ➔ Physical Display.
And:
`Hardware Mouse/Keyboard` ➔ `Input Core` ➔ `Pointer Engine / Event Dispatcher` ➔ `BWE Window Hit-Test` ➔ `sys_gui_post_event()` ➔ Per-window Ring 3 Event Queue ➔ `SYS_GUI_POLL_EVENT` ➔ `gui_demo`.

## 2. Modifications by File

### A. `kernel/core/syscall/src/services.c`
1. Update `sys_service_gui_map_surface`:
   - Allocate `pages_needed * 4096` bytes using `kmalloc_aligned(pages_needed * 4096, 4096)`.
   - Set `win->control_data.canvas.pixel_buffer = (uint32_t*)kbuf`.
   - For each page `p < pages_needed`, query the physical address via `vmm_translate(g_kernel_pml4, (uint64_t)kbuf + p * 4096)`, and map to `cur->pml4` at `user_virt_base + p * 4096` with `PAGE_USER | PAGE_WRITABLE | PAGE_PRESENT`.
   - Return user virtual address `user_virt_base` and stride `win->control_data.canvas.buffer_w * 4`.
2. Update `sys_service_gui_invalidate`:
   - Mark window dirty (`BWE_InvalidateWindow(win_id)`).
   - Trigger immediate `BWE_Compose()`.
3. Update `sys_service_gui_show_window`:
   - Show/hide window and trigger `BWE_Compose()`.

### B. `kernel/wm/bwe/renderer/bwe_compositor.c`
1. In `BWE_ComposeFrame`:
   - When blitting `win->control_data.canvas.pixel_buffer`, log forensic trace marker `[BWE_GUI] SURFACE COMPOSITE PASS`.

### C. `kernel/wm/bwe/src/bwe_core.c`
1. In `BWE_PumpEvents`:
   - When a mouse or keyboard event is hit-tested and routed to a window, log forensic trace markers `[BWE_GUI] HITTEST PASS` and `[BWE_GUI] EVENT ROUTE PASS`.
   - Ensure `sys_gui_post_event` queues the event cleanly.

### D. `kernel/kernel.c`
1. Update the Ring 3 bytecode sequence in `kernel.c`:
   - Create window (600x400).
   - Map surface.
   - Paint high-contrast pattern (slate blue background + inner content cards).
   - Invalidate window.
   - Show window.
   - Poll event loop: read event from `SYS_GUI_POLL_EVENT` (22), log `[RING3_GUI] MOUSE EVENT RECEIVED` / `[RING3_GUI] KEY EVENT RECEIVED` / `[RING3_GUI] DRAG PASS`, yield and loop.

### E. `userspace/apps/gui_demo/main.c`
1. Align userspace C application with identical deterministic markers and full pattern drawing.

## 3. Expected Result
- Zero kernel crashes, zero regressions.
- BWE Compositor blits Ring 3 window surface directly into the display backbuffer and presents to hardware GOP.
- Mouse movement and clicks over the window post events to the window event queue.
- `gui_demo` reads events via `SYS_GUI_POLL_EVENT` and executes continuously at CPL=3.
