# ATOMS OS — BOS C++ UI FRAMEWORK
## PHASE 3: NATIVE BOS WINDOW EXPERIENCE — FORENSIC AUDIT REPORT

**Date:** 2026-09-10  
**Phase:** Phase 3 of 5 (Native BOS Window Experience)  
**Author:** ATOMS Forensic Team  
**Status:** AUDIT COMPLETE — NO SOURCE CODE MODIFIED (Phase Isolation Rule 0 Compliant)

---

## 1. Executive Summary

Phase 1 (BOS C++ UI Foundation) and Phase 2 (Modern Visual Controls + RFC 1951 Deflate/PNG Engine) successfully established freestanding C++ userspace capabilities with 14 visual controls, 9-slice rendering, and alpha composition. However, C++ applications currently interact with the desktop as an opaque client rectangle without first-class native window integration:
1. Window chrome (title bar, borders, shadows) is rendered by the kernel BWE compositor, but userspace C++ `Window` has no explicit knowledge of non-client decoration metrics (`top = 35px`, `bottom = 5px`, `left = 5px`, `right = 5px`), causing confusion between client-relative and window-relative coordinates.
2. The window state machine is limited to visible/hidden/closed booleans, lacking formal states: `Normal`, `Minimized`, `Maximized`, `Active`, `Inactive`, `Closing`, `Closed`.
3. Native window lifecycle operations (`minimize()`, `maximize()`, `restore()`, `close()`) and window-level focus transitions are not exposed to C++ applications.
4. Multi-window coordination (focus switching, z-order activation, multi-window event dispatching) is not fully tied into the C++ `Application` and `Window` abstractions.

This forensic audit inspected the authoritative kernel window manager (`kernel/wm/bwe/`, `kernel/wm/surface/`, `kernel/wm/compositor/`), userspace GUI runtime (`userspace/libbos_gui/`, `userspace/runtime/c/`), and the C++ UI SDK (`sdk/include/bos/`, `sdk/src/bos/ui/`) to determine the exact contract for Phase 3.

---

## 2. Comprehensive Forensic Audit Findings (15 Mandatory Questions)

### Question 1: How native BOS windows are represented
* **Kernel Level:** Native windows are represented as `BWE_Window` objects managed within `s_window_pool[BWE_MAX_WINDOWS]` in `kernel/wm/bwe/src/bwe_window.c`. Each window has a unique 32-bit ID (`win_id`), where `win_id & 0xFFF` indexes the slot. The legacy `BWE_Surface` in `kernel/wm/surface/surface.c` (`surface_pool[MAX_SURFACES]`) mirrors this handle.
* **Syscall Interface:** `SYS_GUI_CREATE_WINDOW` (Syscall 16) returns the 32-bit `win_id`.
* **C++ Level:** `bos::Window` (`sdk/include/bos/window.hpp`) holds `m_window_id`, which maps 1:1 to the native kernel window handle.

### Question 2: How their bounds are stored
* **Kernel Level:** Two distinct rectangles are maintained in `BWE_Window`:
  - `screen_bounds`: Global absolute framebuffer coordinates `(x, y, width, height)`.
  - `local_bounds`: Coordinates relative to the parent surface (for top-level windows, parent is `BWE_DESKTOP_ID = 0`).
  - `restore_bounds`: Stores the pre-maximized normal geometry for restore operations (`bwe_layout.c:L494`).
* **C++ Level:** Stored in `m_bounds` (`bos::Rect(x, y, width, height)`).

### Question 3: How client and content areas are defined
* **Authoritative Metrics:** Defined in `kernel/wm/bwe/src/bwe_geometry.c:L18-L23` via `BWE_Geometry_GetDecorationMetrics`:
  - Top: `35px` (5px outer border + 30px title bar)
  - Bottom: `5px`
  - Left: `5px`
  - Right: `5px`
* **Non-Client vs Client Geometry:**
  - Total Window Bounds: `(x, y, W, H)`
  - Client Bounds: `(x + 5, y + 35, W - 10, H - 40)`
  - Borderless Windows (`flags & BWE_WINDOW_BORDERLESS`): Client Bounds equal Window Bounds `(x, y, W, H)`.
* **Compositor Blit:** In `kernel/wm/bwe/renderer/bwe_compositor.c:L516-L530`, the kernel blits the user-space private window surface (`win->control_data.canvas.pixel_buffer`) directly into `(x + 5, y + 35)` with dimensions clamped to `(W - 10, H - 40)`.

### Question 4: How title and frame areas are currently handled
* **Rendering:** Owned exclusively by the kernel BWE compositor (`kernel/wm/bwe/renderer/bwe_compositor.c:L491-L513`).
  - Shadows: `BWE_DrawShadow(ram_fb, &win->screen_bounds, active)`
  - Borders: `BWE_DrawBorder(ram_fb, &win->screen_bounds, border_color, active)`
  - Title Bar: `BWE_DrawTitleBar(ram_fb, &win->screen_bounds, title_text, active, resizable)` (`bwe_paint.c:L588`)
  - Title text is drawn from `win->title` using `BOFONT_ROLE_UI_BOLD`.
  - Rounded top corners with radius `r = 6px`.
* **Title Updates:** `BOS_SetText(win_id, text)` in `kernel/wm/bwe/src/bwe_core.c:L961` updates `win->title` and dirties the window.

### Question 5: How mouse hit testing works
* **Kernel Function:** `BWE_HitZone BWE_HitTest(uint32_t window_id, int32_t screen_x, int32_t screen_y)` in `kernel/wm/bwe/src/bwe_window.c:L629-L708`.
* **Priority Zones:**
  1. `BWE_HIT_CORNER_TL / TR / BL / BR`: 12x12px active corners.
  2. `BWE_HIT_BORDER_T / B / L / R`: 5px outer perimeter bands.
  3. Titlebar & Control Buttons (`screen_y >= ty && screen_y < ty + 30`):
     - `BWE_HIT_CLOSE`: `close_x = tx + tw - 26`, width 20px, height 20px.
     - `BWE_HIT_MAX`: `max_x = tx + tw - 48`, width 20px, height 20px.
     - `BWE_HIT_MIN`: `min_x = tx + tw - (resizable ? 70 : 48)`, width 20px, height 20px.
     - `BWE_HIT_TITLEBAR`: Remainder of title bar body (`screen_x >= tx && screen_x < tx + tw`).
  4. `BWE_HIT_CLIENT`: Interior area inside decorations.

### Question 6: How window dragging currently works
* **Engine:** In `kernel/wm/bwe/src/bwe_window.c:L754-L797` (`BWE_ProcessMouseInteraction`).
* **Behavior:**
  - Mouse down on `BWE_HIT_TITLEBAR` initiates drag (`s_is_dragging = true`).
  - Anchor offset recorded: `s_drag_offset_x = mouse_x - win->screen_bounds.x`, `s_drag_offset_y = mouse_y - win->screen_bounds.y`.
  - Mouse move updates bounds via `BOS_SetBounds` with screen boundary clamping.
  - Mouse release ends drag (`s_is_dragging = false`).
  - While dragging is active, child control hit testing and event routing are suppressed (`bwe_core.c:L539`).

### Question 7: How resize currently works
* **Engine:** In `kernel/wm/bwe/src/bwe_window.c:L763-L826` (`BWE_ProcessMouseInteraction`).
* **Behavior:**
  - Mouse down on any border or corner initiates 8-way resize (`s_is_resizing = true`, `s_resize_zone = hit`).
  - Start coordinates and bounds captured: `s_resize_start_bounds = win->local_bounds`.
  - Mouse move dynamically computes `dx` and `dy`, modifying width/height and origin `(x, y)` based on active edge/corner.
  - Minimum bounds constraints (`min_size`) are strictly enforced.
  - Mouse release terminates resize (`s_is_resizing = false`).

### Question 8: How focus is assigned
* **Engine:** `BOS_SetFocus(uint32_t window_id)` in `kernel/wm/bwe/src/bwe_window.c:L439-L480`.
* **State Transition:**
  - Previous focused window receives `BWE_STATE_DEACTIVATED` and `BWE_EVENT_FOCUS_LOSS`.
  - New window receives `BWE_STATE_ACTIVE` and `BWE_EVENT_FOCUS_GAIN`.
  - `g_focused_window_id` and `g_active_window_id` are updated.
  - Window is brought to front of z-order stack.
  - Compositor recomposes with active title gradient (`0xFF0058EE` border) vs inactive gradient (`0xFF475569` border).

### Question 9: How z-order is maintained
* **Engine:** `g_z_order_stack[BWE_MAX_WINDOWS]` and `g_z_stack_count` in `kernel/wm/bwe/src/bwe_window.c`.
* **Bring To Front:** `BWE_BringToFront(uint32_t window_id)` relocates the window ID to the top of the stack and increments `g_z_order_version`.
* **Compositor:** Back-to-front painter's algorithm renders stack indices from `0` to `g_z_stack_count - 1`.

### Question 10: How minimize/maximize/restore are represented
* **Maximize:** `BWE_WindowMaximize(window_id)` (`bwe_layout.c:L489`):
  - Saves `win->restore_bounds = win->screen_bounds`.
  - Sets `win->flags |= BWE_WINDOW_FULLSCREEN`.
  - Calls `BOS_SetBounds(window_id, 0, 0, screen_w, screen_h - 32)` (leaving 32px for taskbar).
* **Restore:** `BWE_WindowRestore(window_id)` (`bwe_layout.c:L500`):
  - Clears `win->flags &= ~BWE_WINDOW_FULLSCREEN`.
  - Restores geometry via `BOS_SetBounds` to `restore_bounds`.
* **Minimize:** `BOS_MinimizeSurface(window_id)` (`surface.c:L556`):
  - Sets `surface->state = BWE_STATE_MINIMIZED`.
  - Clears `surface->flags &= ~BWE_FLAG_VISIBLE`.
  - Releases focus via `BOS_ClearFocus()`.

### Question 11: How close requests are delivered
* **Current State:** In `kernel/wm/bwe/src/bwe_window.c:L730`, clicking the close button currently executes `BOS_DestroySurface(win_id)` directly, destroying the window immediately without notifying the userspace event loop!
* **Forensic Requirement:** A close request must post `BOS_GUI_EVENT_CLOSE` to the process event queue via `sys_gui_post_event(win_id, &gui_ev)`. This permits userspace applications to run `on_close` callbacks, confirm closure, save state, and cleanly call `destroy_native_window()`.

### Question 12: What BWE owns
* Authoritative window tree, parent/child hierarchy, window states, window object pool.
* Native window chrome rendering (title bar, buttons, borders, shadows, corner rounding).
* Window dragging state machine and 8-way resize state machine.
* Hit testing subsystem (`BWE_HitTest`).
* Focus engine (`BOS_SetFocus`, active vs inactive styling).
* Z-order stack management (`BWE_BringToFront`, `g_z_order_stack`).

### Question 13: What BCM owns
* Authoritative compositor loop and dirty rectangle tracking (`BWE_AddCompositorDirtyRect`).
* Back-to-front desktop composition of wallpaper, window chrome, and client surfaces.
* Blitting private window pixel buffers (`canvas.pixel_buffer`) to RAM framebuffer and hardware presentation.

### Question 14: What BOSurface owns
* High-level C GUI API (`BOS_CreateWindow`, `BOS_CreateButton`, `BOS_CreatePanel`, etc.).
* Surface descriptor pool (`surface_pool[MAX_SURFACES]`).
* System call dispatch bridge connecting Ring 3 userspace to BWE/BCM subsystems.

### Question 15: Which parts are implemented but not cleanly exposed to C++
1. **Client Area vs Non-Client Area Geometry:** C++ `Window` currently treats the entire window width/height as client space, ignoring the 35px titlebar and 5px borders.
2. **Window Lifecycle Operations:** `minimize()`, `maximize()`, `restore()`, `activate()`, `close()` lack clean C++ APIs.
3. **Window State Model:** `WindowState` (Normal, Minimized, Maximized, Active, Inactive, Closing, Closed) does not exist in C++ `Window`.
4. **Lifecycle Callbacks:** `on_activate()`, `on_deactivate()`, `on_close()` are missing or incomplete.
5. **Event Mapping:** `BOS_GUI_EVENT_FOCUS_GAIN` (8) and `BOS_GUI_EVENT_FOCUS_LOST` (9) are not mapped to `EventType::FocusGained` and `EventType::FocusLost` in `application.cpp`.
6. **PNG/Icon Integration:** Window icon integration with the Phase 2 `Image`/`Icon` system is missing.
7. **DPI Awareness:** Window chrome metrics and client content need unified DPI scaling factors.

---

## 3. Pointer / Cursor Behavior Forensic Assessment

* **Inspection Target:** `kernel/graphics/BSPE/Cursor/bspe_cursor_present.h`, `kernel/wm/bwe/renderer/bwe_compositor.c`.
* **Findings:**
  - The BSPE (BOS Sub-Pixel Engine) Cursor Presenter maintains a single active cursor sprite bitmap rendered either via software VRAM damage blitting or hardware cursor plane.
  - There is currently **no kernel syscall or API** to dynamically switch cursor glyphs (e.g. `CURSOR_RESIZE_NWSE`, `CURSOR_RESIZE_NESW`, `CURSOR_RESIZE_WE`, `CURSOR_RESIZE_NS`, `CURSOR_HAND`).
* **Documented Limitation:** In compliance with Requirement 15 of the specification, multi-shape cursor switching will **not** be faked. This limitation is officially recorded as an existing kernel platform limitation.

---

## 4. Risk Analysis

| Risk Area | Severity | Impact | Mitigation Strategy |
| :--- | :--- | :--- | :--- |
| **C ABI Regression** | HIGH | Breaking `libbos_gui`, `gui_demo.elf`, or `desktop_shell` | Preserve all existing `sys_gui_*` syscall prototypes and semantics exactly. C apps remain 100% unaffected. |
| **Second Window Manager Anti-Pattern** | HIGH | Divergent z-order, duplicate rendering, lag | Enforce BWE/BCM as the sole authority. C++ `Window` only wraps native handles and coordinates client rendering. |
| **Coordinate Drift on Drag/Resize** | MEDIUM | Misaligned widget hit-tests | Distinguish `window_bounds()` from `client_bounds()`. Pass client-relative coordinates to widgets. |
| **Surface Reallocation Glitches** | MEDIUM | Memory leaks or heap crashes during rapid resize | Reuse allocated surface buffers up to the mapped maximum slot capacity (8MB); enforce atomic stride and bounds updates. |

---

## 5. Architectural Recommendations for Phase 3

1. **Extend `bos::Window` in `sdk/include/bos/window.hpp`:**
   - Add `WindowState` enum (`Normal`, `Minimized`, `Maximized`, `Active`, `Inactive`, `Closing`, `Closed`).
   - Add `client_bounds()`, `client_size()`, `title_bar_height()`, `border_thickness()`.
   - Add `minimize()`, `maximize()`, `restore()`, `activate()`.
   - Add `is_minimized()`, `is_maximized()`, `is_active()`, `window_state()`.
   - Add `set_on_activate()`, `set_on_deactivate()`, and extend `set_on_close()`.
   - Add `set_icon(const Image&)` using Phase 2 PNG/Image engine.
2. **Update Event Dispatching in `sdk/src/bos/ui/application.cpp`:**
   - Map `BOS_GUI_EVENT_FOCUS_GAIN` (8) -> `EventType::FocusGained`.
   - Map `BOS_GUI_EVENT_FOCUS_LOST` (9) -> `EventType::FocusLost`.
   - Route activation/deactivation events to target `Window` instances to update `m_active` and fire callbacks.
3. **Coordinate System Unification:**
   - Ensure client widget tree receives coordinates adjusted for non-client frame margins (`top = 35`, `left = 5`, etc.).
   - Support DPI scale multiplier across chrome metrics and widget bounds.
4. **Multi-Window Validation App:**
   - Create `userspace/apps/window_experience_demo/main.cpp` creating a Main Window and a Secondary Window with Phase 2 controls, testing drag, resize, minimize, maximize, restore, focus switching, and z-order.

---

**Forensic Verdict:** AUDIT COMPLETE. Proceeding to Task 2 (Architecture Patch Plan).
