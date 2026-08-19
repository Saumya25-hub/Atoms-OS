# PATCH PLAN — DESKTOP SHELL DIRTY-REGION OPTIMIZATION

## Target Files
1. [`kernel/core/syscall/src/services.c`](file:///d:/Signatures_OS/kernel/core/syscall/src/services.c)
2. [`userspace/apps/desktop_shell/main.c`](file:///d:/Signatures_OS/userspace/apps/desktop_shell/main.c)

---

## Detailed Modifications

### 1. `kernel/core/syscall/src/services.c`
- **Modification**: Update `sys_service_gui_invalidate(uint32_t win_id, int32_t x, int32_t y, int32_t w, int32_t h)` to process sub-rectangle invalidation parameters instead of discarding them.
- **Implementation**:
  ```c
  uint64_t sys_service_gui_invalidate(uint32_t win_id, int32_t x, int32_t y, int32_t w, int32_t h) {
    if (!BWE_ValidateWindow(win_id)) return SYSCALL_FAIL;
    BWE_Window *win = BWE_GetWindow(win_id);
    if (!win) return SYSCALL_FAIL;

    if (w > 0 && h > 0 && (w < win->screen_bounds.width || h < win->screen_bounds.height)) {
      BWE_Rect sub_rect = { win->screen_bounds.x + x, win->screen_bounds.y + y, w, h };
      extern void BWE_AddCompositorDirtyRect(const BWE_Rect* rect);
      BWE_AddCompositorDirtyRect(&sub_rect);
      win->is_dirty = true;
    } else {
      BWE_InvalidateWindow(win_id);
      BWE_RequestFullRedraw();
    }
    return SYSCALL_OK;
  }
  ```
- **Rationale**: Enables $97.8\% - 98.9\%$ VRAM bandwidth reduction per hover frame by forwarding sub-rectangles directly to the kernel BWE compositor dirty rect engine (`BWE_AddCompositorDirtyRect`).

### 2. `userspace/apps/desktop_shell/main.c`
- **Modification**: Track `s_prev_hovered_icon` and issue targeted sub-rectangle invalidations `sys_gui_invalidate(win_id, x, y, w, h)` for icon cards, start button, and start menu popup.
- **Implementation**:
  - For Icon Hover: Invalidate `g_icons[i].x - 6, g_icons[i].y - 6, 92, 92` for both current and previously hovered icon.
  - For Start Button: Invalidate `12, tb_y + 8, 88, 32`.
  - For Start Menu Popup: Invalidate `12, tb_y - 326, 260, 368`.
- **Rationale**: Replaces full-screen 3.14MB/8.29MB invalidations with precise 33.8KB–67.7KB sub-rectangle dirtying, eliminating unnecessary VRAM blits while preserving 100% visual correctness.

---

## Risk & Rollback Plan
- **Risk**: Minimal. Fallback to full-screen invalidation remains active if `w <= 0` or `h <= 0`.
- **Rollback**: Git checkout of `services.c` and `desktop_shell/main.c`.
