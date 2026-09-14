# ATOMS OS — BOS C++ UI FRAMEWORK
## PHASE 3: NATIVE BOS WINDOW EXPERIENCE — ARCHITECTURE PATCH PLAN

**Date:** 2026-09-10  
**Phase:** Phase 3 of 5 (Native BOS Window Experience)  
**Author:** ATOMS Architecture Team  
**Status:** PROPOSED PLAN — AWAITING USER APPROVAL (Rule 0 Phase Isolation)

---

## 1. Architectural Philosophy & Guarantees

In accordance with Section 1 of the Phase 3 specification:
* **The existing native ATOMS window architecture (BOSurface / BWE / BCM) is authoritative.**
* **NO SECOND WINDOW MANAGER:** The C++ framework will NOT implement another window server, compositor, z-order manager, or independent hit-testing tree.
* **BWE/BCM Authority:** BWE owns window chrome, dragging, resizing, focus, and z-order. BCM owns layer composition and dirty damage tracking.
* **C++ Role:** Expose, integrate with, and properly coordinate the native BOS window capabilities from C++, providing a first-class desktop window experience.
* **Zero C ABI Regressions:** C applications (`libbos_gui`, `gui_demo.elf`, `sdk_explorer.elf`, `desktop_shell`) remain 100% operational with identical binary and source interfaces.

---

## 2. Target Files for Modification & Creation

| File Path | Action | Layer | Purpose |
| :--- | :--- | :--- | :--- |
| `sdk/include/bos/window.hpp` | MODIFY | C++ UI SDK | Add `WindowState`, client area metrics, lifecycle methods (`minimize`, `maximize`, `restore`, `activate`), callbacks, and icon storage. |
| `sdk/src/bos/ui/window.cpp` | MODIFY | C++ UI SDK | Implement client vs non-client coordinate translation, window state transitions, lifecycle actions, and event routing. |
| `sdk/src/bos/ui/application.cpp` | MODIFY | C++ UI SDK | Map `BOS_GUI_EVENT_FOCUS_GAIN` / `FOCUS_LOST`, track `active_window()`, and coordinate multi-window event distribution. |
| `userspace/apps/window_experience_demo/main.cpp` | CREATE | Application | Dedicated production-grade multi-window test app testing Main + Secondary windows with full Phase 2 controls, drag, resize, and state transitions. |
| `build.ps1` | MODIFY | Build System | Register `window_experience_demo.elf` build rule and verify zero compilation/link errors. |
| `docs/cplusplus_ui/PHASE3_PATCH_REPORT.md` | CREATE | Documentation | Detailed report of changes applied by Patch Team. |
| `docs/cplusplus_ui/PHASE3_CERTIFICATION_REPORT.md` | CREATE | Documentation | Formal PASS/FAIL certification with regression matrix. |

---

## 3. Detailed Component Plan

### 3.1. C++ Window API Extensions (`sdk/include/bos/window.hpp`)

```cpp
namespace bos {

// Formal Window State Model
enum class WindowState : uint32_t {
    Normal = 0,
    Minimized,
    Maximized,
    Active,
    Inactive,
    Closing,
    Closed
};

class Window {
public:
    using CloseCallback      = void (*)(Window*);
    using ResizeCallback     = void (*)(Window*, Size);
    using ActivateCallback   = void (*)(Window*);
    using DeactivateCallback = void (*)(Window*);

    // Existing methods preserved (100% Phase 1/2 compatible)...

    // State Inspection
    WindowState window_state() const { return m_state; }
    bool is_minimized() const { return m_state == WindowState::Minimized; }
    bool is_maximized() const { return m_state == WindowState::Maximized; }
    bool is_active() const { return m_active; }

    // Client vs Non-Client Metrics
    Rect client_bounds() const;
    Size client_size() const;
    static int32_t title_bar_height();
    static int32_t border_thickness();
    Point window_to_client(const Point& pt) const;
    Point client_to_window(const Point& pt) const;

    // Window Lifecycle Operations
    void minimize();
    void maximize();
    void restore();
    void activate();
    void set_active(bool active);

    // Callbacks
    void set_on_activate(ActivateCallback cb) { m_on_activate = cb; }
    void set_on_deactivate(DeactivateCallback cb) { m_on_deactivate = cb; }

    // Icon Integration (Phase 2 Image)
    void set_icon(const Image& icon);
    const Image* icon() const { return m_has_icon ? &m_icon : nullptr; }
};

} // namespace bos
```

### 3.2. Client vs Non-Client Coordinate Handling (`sdk/src/bos/ui/window.cpp`)

* **Non-Client Metrics:**
  - Border thickness: `5px`
  - Title bar height: `30px` (Top decoration total: `35px`)
* **Coordinate Mapping:**
  - When mouse events arrive at `Window::dispatch_event(const Event& event)`:
  - If `event.mouse_pos.y < 35 || event.mouse_pos.x < 5 || event.mouse_pos.x >= m_bounds.width - 5 || event.mouse_pos.y >= m_bounds.height - 5`:
    - The click is in the **non-client area** (title bar, window control buttons, or borders).
    - Client widget tree hit testing is bypassed, ensuring client buttons under the title bar are never falsely clicked.
  - For client clicks:
    - `Point client_pt(event.mouse_pos.x - 5, event.mouse_pos.y - 35);`
    - Client widget tree is hit-tested with `client_pt`.
* **State Transitions:**
  - `maximize()`: Queries screen size via `sys_gui_get_screen_info()`, caches `m_normal_bounds = m_bounds`, calls `set_bounds(0, 0, screen_w, screen_h - 32)`, and updates `m_state = WindowState::Maximized`.
  - `restore()`: Restores `set_bounds(m_normal_bounds.x, m_normal_bounds.y, m_normal_bounds.width, m_normal_bounds.height)` and sets `m_state = WindowState::Normal`.
  - `minimize()`: Saves state, calls `hide()`, sets `m_state = WindowState::Minimized`.
  - `activate()`: Brings window to front, sets `m_active = true`, fires `m_on_activate(this)`.

### 3.3. Application Event Routing (`sdk/src/bos/ui/application.cpp`)

* Translate raw kernel GUI event codes:
  - `BOS_GUI_EVENT_FOCUS_GAIN` (8) -> `EventType::FocusGained`
  - `BOS_GUI_EVENT_FOCUS_LOST` (9) -> `EventType::FocusLost`
* Maintain `m_active_window`:
  - When a window receives `FocusGained`, mark it active, mark previously focused window inactive, and trigger their respective callbacks.

### 3.4. Dedicated Multi-Window Showcase (`userspace/apps/window_experience_demo/main.cpp`)

* Create two independent windows:
  1. **Main Window (720x480):** "ATOMS OS — Native Desktop Controller"
     - Visual Card with Icon & Title
     - Control buttons: "Minimize", "Maximize", "Restore", "Open Secondary Window", "Close"
     - Status badge showing: State (Normal/Maximized/Minimized), Focus (Active/Inactive), Dimensions
     - Phase 2 Controls: ProgressBar, Slider, Toggle, Checkbox, TextBox, ListView
  2. **Secondary Window (480x360):** "ATOMS Inspector — System Diagnostics"
     - Real-time diagnostic cards showing window coordinates, client bounds, and focus logs
     - Interactivity buttons testing focus switching and state changes between windows
* Test Scenarios to Verify:
  - Focus switching by clicking between Main and Secondary windows
  - Dragging either window without lag or visual corruption
  - 8-way resizing of both windows
  - Minimize, Maximize, and Restore operations
  - Closing Secondary window while Main window continues running smoothly

---

## 4. Rollback Plan

If any regression occurs during build or runtime validation:
1. Revert `sdk/include/bos/window.hpp` and `sdk/src/bos/ui/window.cpp` to git HEAD.
2. Remove `userspace/apps/window_experience_demo/` and revert `build.ps1`.
3. Re-run Phase 1 (`cpp_ui_demo.elf`), Phase 2 (`settings_demo.elf`), and C (`sdk_explorer.elf`) builds to confirm baseline restoration.

---

## 5. Verification & Acceptance Criteria

1. **Clean Freestanding Build:** `libbos_ui_cpp.a` and `window_experience_demo.elf` compile with zero errors and zero warnings.
2. **Zero Regressions:** `cpp_ui_demo.elf`, `settings_demo.elf`, and `sdk_explorer.elf` link cleanly.
3. **Multi-Window Validation:** Both Main and Secondary windows instantiate, render chrome, respond to drag, resize, focus switching, minimize, maximize, restore, and close.
4. **Coordinate Accuracy:** Widget controls hit test accurately within the client area without non-client overlap.
