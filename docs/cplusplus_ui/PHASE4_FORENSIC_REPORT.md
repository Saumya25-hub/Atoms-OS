# ATOMS OS — BOS C++ UI FRAMEWORK
## PHASE 4: EVENTS + LAYOUT + THEME + ANIMATION + POLISH — FORENSIC AUDIT REPORT

**Date:** 2026-09-10  
**Phase:** Phase 4 of 5 (Windows-Level UI Behaviour & Polish)  
**Author:** ATOMS Forensic Team  
**Status:** AUDIT COMPLETE — NO SOURCE CODE MODIFIED (Phase Isolation Rule 0 Compliant)

---

## 1. Executive Summary

Phases 1 through 3 successfully established the freestanding C++ userspace foundations (`Application`, `Window`, `Surface`, `Widget`, `Event`), modern visual controls suite (14 controls with RFC 1951 Deflate/PNG chunk decoding and 9-slice rendering), and native desktop window integration (authoritative BWE/BCM chrome, non-client isolation, dragging, 8-way resizing, and multi-window state management).

However, controls currently behave as independent visual elements manually placed with hardcoded coordinates, without a cohesive application-level behavior loop:
1. **Event Routing & Capture:** Mouse interactions are dispatched directly by coordinate hit testing without capture semantics (e.g. dragging off a pressed button cancels its activation), and keyboard focus lacks a global Tab / Shift+Tab traversal manager.
2. **Layout Engine:** Only primitive `ManualLayout` and basic `StackLayout` exist in `sdk/include/bos/layout.hpp`. Responsive resizing, linear flows (horizontal/vertical box layouts), anchor positioning, margins, padding, and stretch factors are absent, forcing applications to manually position widgets with absolute coordinates.
3. **Theme System:** `Theme` (`sdk/include/bos/theme.hpp`) is currently a static collection of compile-time dark colors, lacking dynamic switching between Light and Dark themes, centralized design tokens, and high-visibility focus indicators.
4. **Animation / Transitions:** No non-blocking animation or easing infrastructure exists; transitions for hover, focus rings, progress bars, and scrolling must be executed manually or remain static.
5. **Dirty Regions & Redraw Optimization:** Redraws currently invalidate full widget or window bounds rather than maintaining an aggregated damage rectangle tracker.

This forensic audit inspected the existing SDK headers (`sdk/include/bos/`), implementation files (`sdk/src/bos/ui/`), kernel event subsystems, and existing demo applications to establish the exact architecture for Phase 4.

---

## 2. Forensic Audit of Existing Subsystems

### 2.1. Event System & Focus Management
* **Current State:** Defined in `sdk/include/bos/events.hpp` and `sdk/src/bos/ui/window.cpp:L318-L380`.
* **Findings:**
  - `EventType` covers basic mouse and key events, but lacks `MouseWheel`, `MouseCapture`, and formal event phases (`Capture`, `Target`, `Bubble`).
  - Mouse down captures are local to each control; moving the mouse cursor outside a button while pressed does not properly maintain capture or suppress click on release outside.
  - Tab and Shift+Tab key codes are delivered as generic `KeyDown` events; there is no centralized `FocusManager` traversing focusable widgets (`Widget::focusable()`).
  - Active focus visualization relies solely on control-specific border colors, without an accessible, high-contrast focus ring.

### 2.2. Layout Engine & Responsive Sizing
* **Current State:** Defined in `sdk/include/bos/layout.hpp`.
* **Findings:**
  - Contains only `Layout` interface, `ManualLayout`, and `StackLayout`.
  - No `LinearLayout` (Horizontal/Vertical box distribution), `AnchorLayout` (docking edges and centering), or `GridLayout`.
  - `Insets` (`geometry.hpp:L47-L67`) supports `left, top, right, bottom`, but is not utilized for widget margins or padding in layout calculation.
  - When a window is resized via native BWE 8-way resizing, root widgets must manually calculate and re-assign every child coordinate, or remain fixed.

### 2.3. Theme System & Tokens
* **Current State:** Defined in `sdk/include/bos/theme.hpp`.
* **Findings:**
  - Pure static `constexpr` dark palette (`0xFF0B0F19`, `0xFF161F30`, `0xFF1E293B`, etc.).
  - No `ThemeMode` enum (`Dark`, `Light`) or dynamic `ThemeManager`.
  - In Light mode, background and surface tokens must adapt to clean, high-contrast slate/white palettes, with dark text and accessible focus indicators.
  - Controls must be able to listen for theme change events or query active theme tokens dynamically.

### 2.4. Typography & Font Metrics
* **Current State:** `bovisual/Text/font8x16.h` in `sdk/src/bos/ui/surface.cpp:L315-L380`.
* **Findings:**
  - Fixed 8x16 bitmap font engine (`g_font8x16_stub`).
  - Character width is constant (8px) and height is constant (16px).
  - Lacks structured `Typography` role tokens (`Heading`, `Title`, `Body`, `Caption`, `Button`, `Input`), baseline alignment, and text ellipsis (`"..."`) when string length exceeds available width.

### 2.5. Animation & Timing Engine
* **Current State:** No animation classes exist in the C++ UI SDK.
* **Findings:**
  - Timing is available via `rdtsc` or `SYS_UPTIME` (Syscall 4).
  - No non-blocking `Animation` / `Animator` framework with easing functions (`Linear`, `EaseIn`, `EaseOut`, `EaseInOut`).
  - An animation engine must execute within `Application::run()` without blocking the event loop or spinning.

### 2.6. Invalidation, Clipping & Dirty Regions
* **Current State:** `Surface::set_clip`, `Surface::clip()`, and `Window::invalidate(const Rect&)`.
* **Findings:**
  - `Widget::paint` already performs hierarchical clipping: `surface.set_clip(prev_clip.intersect(abs_r))`.
  - However, dirty regions trigger full-window redraws or coarse rect updates; a damage accumulator can coalesce multiple dirty rectangles to minimize framebuffer blits.

---

## 3. Files Involved in Phase 4 Scope

| Subsystem | Existing Files | Planned Files / Modifications |
| :--- | :--- | :--- |
| **Event System** | `sdk/include/bos/events.hpp`, `sdk/src/bos/ui/window.cpp` | Extend `EventType`, add `EventPhase`, add mouse capture & Tab focus traversal. |
| **Layout Engine** | `sdk/include/bos/layout.hpp` | Implement `LinearLayout` (HBox/VBox), `AnchorLayout`, `GridLayout`, margin/padding support. |
| **Theme System** | `sdk/include/bos/theme.hpp` | Implement `ThemeMode` (`Dark`, `Light`), dynamic theme manager, centralized design tokens, and focus ring metrics. |
| **Typography** | `sdk/include/bos/types.hpp`, `sdk/src/bos/ui/surface.cpp` | Add `Typography` abstraction, scaled font metrics, and text truncation/ellipsis. |
| **Animation Engine** | *None* | Add `sdk/include/bos/animation.hpp` with non-blocking easing, progress, and animator state. |
| **Showcase App** | `userspace/apps/settings_demo/` | Create `userspace/apps/ui_behavior_demo/main.cpp` demonstrating responsive reflow, Tab navigation, theme toggling, and animations. |

---

## 4. Risk Analysis

| Risk Area | Severity | Impact | Mitigation Strategy |
| :--- | :--- | :--- | :--- |
| **Heap Allocations in Hot Paths** | HIGH | GC pauses or memory exhaustion during animation/resize | Use value types (`Rect`, `Point`, `Insets`), fixed-capacity buffers, and static/stack easing calculators. |
| **Breaking Phase 1–3 Controls** | HIGH | Regression in existing 14 controls or demos | Additive API design: default to existing manual positioning if no layout is attached; preserve all existing constructor signatures. |
| **C ABI Regression** | HIGH | Breaking `sdk_explorer.elf` or `gui_demo.elf` | Touch zero C headers and zero kernel syscall numbers. Keep C runtime pristine. |
| **Blocking Animation Loop** | MEDIUM | UI freezing or stuttering | Animations are pumped per frame via `Application::run()` using delta milliseconds; strictly non-blocking. |

---

## 5. Forensic Verdict

Audit complete. The existing architecture is clean, decoupled, and fully ready for the additive Phase 4 upgrades without breaking Phase 1, 2, or 3. Proceeding to Task 2 (Architecture Patch Plan).
