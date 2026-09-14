# ATOMS OS — BOS C++ UI FRAMEWORK
## PHASE 4: EVENTS + LAYOUT + THEME + ANIMATION + POLISH — ARCHITECTURE PATCH PLAN

**Date:** 2026-09-10  
**Phase:** Phase 4 of 5 (Windows-Level UI Behaviour & Polish)  
**Author:** ATOMS Architecture Team  
**Status:** PROPOSED PLAN — AWAITING USER APPROVAL (Rule 0 Phase Isolation)

---

## 1. Architectural Principles & Guarantees

In accordance with Section 1 of the Phase 4 specification:
* **Preserve Native BOS Architecture:** BWE/BCM/BOSurface remains the authoritative Ring-0 window manager and compositor.
* **No Second Window Manager / Compositor:** Phase 4 operates strictly as the userspace framework application layer.
* **Freestanding C++20 Compliance:** No exceptions (`-fno-exceptions`), no RTTI (`-fno-rtti`), no hosted libc/libstdc++ dependencies. Zero heap allocations in event routing or animation frames.
* **Additive / Backward Compatible:** Phase 1 (Foundation), Phase 2 (Controls + PNG Engine), and Phase 3 (Native Window Experience) public APIs remain 100% functional.
* **Zero C ABI Regressions:** `libbos_gui`, `gui_demo.elf`, `sdk_explorer.elf`, and `desktop_shell` remain completely untouched and operational.

---

## 2. Target Files for Creation and Modification

| Target File | Action | Purpose |
| :--- | :--- | :--- |
| `sdk/include/bos/events.hpp` | MODIFY | Add `EventPhase` (`Capture`, `Target`, `Bubble`), `MouseWheel`, and wheel delta support. |
| `sdk/include/bos/animation.hpp` | CREATE | Lightweight, non-blocking animation system with easing (`Linear`, `EaseIn`, `EaseOut`, `EaseInOut`) and `Animator`. |
| `sdk/include/bos/layout.hpp` | MODIFY | Implement `LinearLayout` (HBox/VBox), `AnchorLayout`, `GridLayout`, margin/padding layout logic. |
| `sdk/include/bos/widget.hpp` | MODIFY | Add margins, padding, content bounds, layout attachment, mouse capture hooks, and Tab focus index. |
| `sdk/src/bos/ui/widget.cpp` | MODIFY | Implement layout calculation, recursive Tab focus search, and hierarchical content clipping. |
| `sdk/include/bos/theme.hpp` | MODIFY | Implement `ThemeMode` (`Dark`, `Light`), dynamic color tokens, and accessible focus ring metrics. |
| `sdk/src/bos/ui/theme.cpp` | CREATE | Dynamic theme manager implementation and state style generators for Light and Dark modes. |
| `sdk/src/bos/ui/window.cpp` | MODIFY | Implement mouse capture routing, Tab / Shift+Tab keyboard focus traversal, and responsive reflow on resize. |
| `sdk/src/bos/ui/application.cpp` | MODIFY | Hook non-blocking animation timer updates into main event loop. |
| `userspace/apps/ui_behavior_demo/main.cpp` | CREATE | Dedicated Phase 4 showcase app testing all 12 required interaction tests (A through L). |
| `build.ps1` | MODIFY | Add build targets for `theme.cpp` and `ui_behavior_demo.elf`. |
| `docs/cplusplus_ui/PHASE4_PATCH_REPORT.md` | CREATE | Patch Team implementation record. |
| `docs/cplusplus_ui/PHASE4_CERTIFICATION_REPORT.md` | CREATE | Formal PASS/FAIL certification report. |

---

## 3. Detailed Component Architecture

### 3.1. Event Routing & Focus Navigation (`events.hpp`, `window.cpp`)
* **Mouse Capture:**
  - `Window::set_mouse_capture(Widget* w)` captures all mouse move and mouse up events to `w` until mouse up occurs.
  - If mouse release occurs outside `w->bounds()`, click activation is cancelled (standard OS button capture behavior).
* **Keyboard Tab Navigation:**
  - Pressing `Tab` triggers `Window::focus_next_widget()`.
  - Pressing `Shift+Tab` triggers `Window::focus_prev_widget()`.
  - Recursively traverses widgets where `focusable() == true` sorted by `tab_index()`.
  - Focused widget receives `FocusGained` and draws an accessible focus ring (`Theme::FocusRing()`).

### 3.2. Responsive Layout Engine (`layout.hpp`, `widget.cpp`)
* **LinearLayout (HBox / VBox):**
  - Distributes children along horizontal or vertical axis with customizable `spacing` and `alignment` (`Start`, `Center`, `End`, `Stretch`).
  - Automatically calculates child bounds within the container's `content_bounds() = bounds().inset(padding())`.
* **AnchorLayout:**
  - Children can anchor to `Left`, `Top`, `Right`, `Bottom`, `CenterHorizontal`, `CenterVertical`, or `Fill`.
  - When the window resizes, anchored controls dynamically stretch or reposition without hardcoded coordinates.
* **GridLayout:**
  - Places controls in a uniform `rows x columns` grid with cell spacing and margin offsets.
* **Responsive Window Resize:**
  - `Window::dispatch_event(EventType::WindowResize)` updates root widget bounds and calls `root_widget->perform_layout()`, causing all child layouts to reflow deterministically.

### 3.3. Dynamic Theme System (`theme.hpp`, `theme.cpp`)
* **Theme Mode Enum:** `ThemeMode::Dark` and `ThemeMode::Light`.
* **Dynamic Palette:**
  - Dark: Background `0xFF0B0F19`, Card `0xFF161F30`, Subtle `0xFF1E293B`, Text `0xFFF8FAFC`, Accent `0xFF2563EB`.
  - Light: Background `0xFFF1F5F9`, Card `0xFFFFFFFF`, Subtle `0xFFE2E8F0`, Text `0xFF0F172A`, Accent `0xFF0058EE`.
  - Focus Ring: `0xFF38BDF8` (Dark) / `0xFF0058EE` (Light) with 2px prominent indicator.
* **Runtime Toggling:** `Theme::toggle_theme()` or `Theme::set_mode(ThemeMode)` instantly updates all active windows and controls.

### 3.4. Non-Blocking Animation System (`animation.hpp`)
* **Easing Functions:**
  - `Linear`: $f(t) = t$
  - `EaseIn`: $f(t) = t^2$
  - `EaseOut`: $f(t) = t \cdot (2 - t)$
  - `EaseInOut`: $f(t) = t < 0.5 ? 2t^2 : -1 + (4 - 2t)t$
* **Execution:**
  - Driven per-frame inside `Application::run()`.
  - Updates values smoothly without blocking or stalling input.
  - Used for smooth progress bar fills, hover fading, and focus transitions.

### 3.5. Dedicated Showcase (`ui_behavior_demo/main.cpp`)
* Implements the 12 required interaction tests:
  - **Test A:** Hover transitions & enter/leave logging.
  - **Test B:** Button press and release activation.
  - **Test C:** Mouse capture cancellation when released outside.
  - **Test D & E:** Tab and Shift+Tab keyboard focus traversal.
  - **Test F:** TextBox keyboard typing and focus caret.
  - **Test G:** ScrollView viewport clipping and scrolling.
  - **Test H:** Continuous window resizing with responsive reflow.
  - **Test I:** Light/Dark theme dynamic switching.
  - **Test J:** Non-blocking animation progress bar & slider.
  - **Test K:** Dirty-region damage clipping.
  - **Test L:** High-contrast accessible focus rings.

---

## 4. Rollback Plan

1. Revert `sdk/include/bos/` and `sdk/src/bos/ui/` to git checkpoint.
2. Remove `userspace/apps/ui_behavior_demo/` and revert `build.ps1`.
3. Rebuild Phase 1–3 targets (`cpp_ui_demo.elf`, `settings_demo.elf`, `window_experience_demo.elf`) and C baseline (`sdk_explorer.elf`) to confirm zero regression.

---

## 5. Verification Plan

1. **Clean Build:** `libbos_ui_cpp.a` and `ui_behavior_demo.elf` compile with zero warnings and zero errors.
2. **Regression Check:** All existing Phase 1–3 demos and C apps build cleanly.
3. **Interactive Verification:** Run automated test harness verifying all 12 interaction scenarios (A through L).
