# PATCH REPORT — MILESTONE 2: BWE COMPOSITOR & INTERACTIVE RING 3 GUI

## 1. Summary of Changes
Completed Milestone 2: Ring 3 private window surface memory allocation, zero-copy physical page mapping, BWE compositor integration, and interactive mouse/keyboard event dispatch to userspace event queue.

## 2. Files Changed

### A. `kernel/core/syscall/src/services.c`
- **Functions Changed**: `sys_service_gui_map_surface`, `sys_service_gui_invalidate`
- **Modifications**:
  - Replaced non-coherent physical page allocation with direct `pmm_alloc_page()` + `vmm_map_page()` to map pages directly to `cur->pml4` at `user_virt_base` (`0x50000000 + win_id * 0x1000000`).
  - Added immediate `BWE_Compose()` invocation in `sys_service_gui_invalidate` so user pixel updates are presented onto the hardware display.

### B. `kernel/wm/bwe/renderer/bwe_compositor.c`
- **Functions Changed**: `BWE_ComposeFrame`
- **Modifications**:
  - Added deterministic marker `[BWE_GUI] SURFACE COMPOSITE PASS` upon compositing client-area surface pixels.

### C. `kernel/wm/bwe/src/bwe_core.c`
- **Functions Changed**: `BWE_PumpEvents`
- **Modifications**:
  - Added deterministic markers `[BWE_GUI] HITTEST PASS` and `[BWE_GUI] EVENT ROUTE PASS` upon dispatching mouse and keyboard events to leaf controls and the Ring 3 event queue.

### D. `kernel/kernel.c`
- **Functions Changed**: `kmain` fallback user process setup
- **Modifications**:
  - Configured full 600x400 window creation, surface mapping, high-contrast pattern rendering, invalidate, show, and continuous event polling loop with `[RING3_GUI]` forensic markers.

### E. `userspace/apps/gui_demo/main.c`
- **Functions Changed**: `main`
- **Modifications**:
  - Updated print markers to standardized `[RING3_GUI]` forensic tokens.
