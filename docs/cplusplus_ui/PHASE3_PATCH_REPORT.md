# ATOMS OS — BOS C++ UI FRAMEWORK
## PHASE 3: NATIVE BOS WINDOW EXPERIENCE — PATCH REPORT

**Date:** 2026-09-10  
**Phase:** Phase 3 of 5 (Native BOS Window Experience)  
**Author:** ATOMS Patch Team  
**Status:** IMPLEMENTATION COMPLETE — 100% BUILD SUCCESS  

---

## 1. Summary of Work

In strict accordance with the approved `PHASE3_PATCH_PLAN.md` and Rule 0 phase isolation protocol, the Patch Team implemented the Native BOS Window Experience layer. The implementation preserves the authoritative kernel BWE/BCM window manager, introduces no duplicate window manager or compositor, and cleanly exposes native windowing capabilities to modern C++ applications.

---

## 2. Files Modified and Created

### 2.1. C++ UI SDK Header Extensions
* **File:** [`sdk/include/bos/window.hpp`](file:///d:/Signatures_OS/sdk/include/bos/window.hpp)
* **Changes:**
  - Added formal `WindowState` enum: `Normal`, `Minimized`, `Maximized`, `Active`, `Inactive`, `Closing`, `Closed`.
  - Added callback types: `ActivateCallback` (`void (*)(Window*)`) and `DeactivateCallback` (`void (*)(Window*)`).
  - Added non-client metrics and coordinate mapping helpers:
    - `Rect client_bounds() const`
    - `Size client_size() const`
    - `static int32_t title_bar_height()` (35px, matching BWE `BWE_Geometry_GetDecorationMetrics`)
    - `static int32_t border_thickness()` (5px, matching BWE decoration margins)
    - `Point window_to_client(const Point& pt) const`
    - `Point client_to_window(const Point& pt) const`
  - Added native window lifecycle methods:
    - `void minimize()`
    - `void maximize()`
    - `void restore()`
    - `void activate()`
    - `void set_active(bool active)`
  - Added state queries:
    - `WindowState window_state() const`
    - `bool is_minimized() const`
    - `bool is_maximized() const`
    - `bool is_active() const`
  - Added Phase 2 PNG/Image icon support:
    - `void set_icon(const Image& icon)`
    - `const Image* icon() const`
  - Added normal bounds cache `m_normal_bounds` for geometry restoration after maximize.

### 2.2. C++ UI Window Implementation
* **File:** [`sdk/src/bos/ui/window.cpp`](file:///d:/Signatures_OS/sdk/src/bos/ui/window.cpp)
* **Changes:**
  - Implemented `client_bounds()`, `client_size()`, `window_to_client()`, `client_to_window()`.
  - Implemented `minimize()`, `maximize()`, `restore()`, `activate()`, `set_active()`, `set_icon()`.
  - Implemented client vs non-client event isolation in `dispatch_event()`:
    - Detected clicks on the native title bar (`y < 35`) and 5px borders.
    - Non-client clicks are suppressed from reaching the client widget tree, ensuring that native dragging and window controls cannot inadvertently trigger underlying application buttons.
    - Client mouse coordinates are translated into client-relative coordinates before passing down the widget hierarchy.
  - Implemented `EventType::FocusGained` and `EventType::FocusLost` handling:
    - Updates `m_active` flag.
    - Fires `m_on_activate` or `m_on_deactivate` user callbacks.
    - Triggers visual redraw with active/inactive state styling.
  - Implemented `EventType::WindowResize` dynamic remapping and root widget bounds synchronization to `client_size()`.

### 2.3. Top-Level Application Event Routing
* **File:** [`sdk/include/bos/application.hpp`](file:///d:/Signatures_OS/sdk/include/bos/application.hpp)
* **File:** [`sdk/src/bos/ui/application.cpp`](file:///d:/Signatures_OS/sdk/src/bos/ui/application.cpp)
* **Changes:**
  - Extended event translation switch statement:
    - `case 8` (`BOS_GUI_EVENT_FOCUS_GAIN`) -> `EventType::FocusGained`
    - `case 9` (`BOS_GUI_EVENT_FOCUS_LOST`) -> `EventType::FocusLost`
  - Added active window tracking:
    - `Window* active_window() const`
    - `Window* window_at(size_t index) const`
    - `void set_active_window(Window* win)`
  - Coordinated multi-window focus switching: deactivates previously active window when a new window gains focus.

### 2.4. Dedicated Multi-Window Showcase Application
* **File:** [`userspace/apps/window_experience_demo/main.cpp`](file:///d:/Signatures_OS/userspace/apps/window_experience_demo/main.cpp)
* **Features:**
  - Dual-window desktop setup:
    1. **Primary Controller (720x480):** Header Card, Action Buttons ("Minimize", "Maximize", "Restore", "Focus Secondary", "Toggle Secondary"), Live Metrics Labels ("Focus", "Bounds", "Client"), Form Controls (CheckBox, Toggle, ProgressBar), and Event Log ListView.
    2. **Diagnostics Inspector (460x340):** Secondary window with Header Card, Action Buttons ("Activate Main", "Minimize Me", "Close Me"), Live Focus Status, TextBox input, and ListView.
  - Interactive callbacks:
    - Dynamic status badge updates on `set_on_activate` / `set_on_deactivate`.
    - Dynamic geometry updates on `set_on_resize`.
    - Cascading shutdown on `set_on_close`.

### 2.5. Build System Registration
* **File:** [`build.ps1`](file:///d:/Signatures_OS/build.ps1)
* **Changes:**
  - Registered `build/window_experience_demo.elf` compilation and linking target.

---

## 3. Build & Artifact Verification

| Target | Command | Result | Notes |
| :--- | :--- | :--- | :--- |
| `build/bos_ui_window.o` | Clang++ 22.1.8 | Code 0 | Freestanding C++20, `-nostdinc -nostdinc++` |
| `build/bos_ui_application.o` | Clang++ 22.1.8 | Code 0 | Freestanding C++20 |
| `build/libbos_ui_cpp.a` | llvm-ar rcs | Code 0 | Static library updated with 19 object files |
| `build/window_experience_demo.elf` | ld.lld | Code 0 | 97,336 bytes, entry `0x400001E0`, zero unresolved symbols |
| `build/settings_demo.elf` | ld.lld | Code 0 | Phase 2 showcase builds cleanly (zero regression) |
| `build/cpp_ui_demo.elf` | ld.lld | Code 0 | Phase 1 demo builds cleanly (zero regression) |
| `build/sdk_explorer.elf` | ld.lld | Code 0 | C application builds cleanly (zero regression) |

---

**Patch Verdict:** SUCCESS. Proceeding to Task 4 (Certification Team).
