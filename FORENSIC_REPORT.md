# 🔬 FORENSIC INVESTIGATION REPORT: FIRST RING 3 GUI PROCESS EXECUTION & EVENT ROUTING
**Subsystem:** BWE Event Pump & Compositor (`bwe_core.c`, `bwe_compositor.c`), Syscall Event Queue (`services.c`), Ring 3 Usermode Launcher (`kernel.c`)  
**Investigating Agent:** Antigravity / ARYA Core Forensic  
**Date:** 2026-08-17  
**Status:** TASK 1 COMPLETE (Forensic Phase — NO CODE MODIFIED)

---

## 1. Forensic Milestone Question
> “Kya ATOMS OS ka first minimal Ring 3 GUI process successfully execute karke, apni private window create, surface map, render, show aur mouse/keyboard event receive kar sakta hai — bina Ring 0 desktop shell ko use kiye?”

---

## 2. Forensic Findings & Subsystem Gap Analysis

1. **Window Surface Compositing (`bwe_compositor.c`):**
   - Current BWE compositor renders titlebars, borders, and child surfaces.
   - When a window is created via `SYS_GUI_CREATE_WINDOW` and mapped via `SYS_GUI_MAP_SURFACE`, its backing buffer `win->control_data.canvas.pixel_buffer` contains the user process's private pixels.
   - **Gap:** The compositor needs to blit `pixel_buffer` into the client area `[x+5, y+35, w-10, h-40]` during `compose_window_recursive()`.

2. **Event Delivery into Ring 3 Event Queue (`bwe_core.c`):**
   - Current `BWE_PumpEvents()` converts raw input into `BWE_Event` and dispatches to `target->on_event`.
   - **Gap:** In addition to internal callbacks, `BWE_PumpEvents()` must translate mouse move, mouse down, mouse up, and keyboard events into `BOS_GUIEvent` and call `sys_gui_post_event(target_id, &ev)`.

3. **Ring 3 GUI Process Execution (`kernel.c` / Process Spawner):**
   - The Level 5 Process Engine already initializes PML4 and transitions to usermode via `iretq` in `ring3.asm`.
   - `userspace/apps/gui_demo/` compiles into `gui_demo.elf` which invokes `sys_gui_create_window`, `sys_gui_map_surface`, draws onto the private surface, calls `sys_gui_show_window`, and loops in `sys_gui_poll_event`.

---

## 3. Files Involved
* `kernel/wm/bwe/renderer/bwe_compositor.c`: Add canvas backing buffer blit in window compositor.
* `kernel/wm/bwe/src/bwe_core.c`: Forward BWE events into `sys_gui_post_event()`.
