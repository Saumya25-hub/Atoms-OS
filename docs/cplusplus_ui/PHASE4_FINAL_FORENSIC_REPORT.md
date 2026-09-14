# ATOMS OS / BOS C++ UI — Phase 4 Final Forensic Report
**USABILITY & SMART LAYOUT ENGINE FORENSIC INVESTIGATION**

**Date:** 2026-09-10  
**Phase:** Phase 4 Final (Usability + Smart Layout + Window Geometry)  
**Status:** FORENSIC INVESTIGATION COMPLETE — NO CODE MODIFIED  

---

## 1. Executive Summary

Following the visual certification of Phase 4A (typography system, anti-aliased Inter font rendering, design token architecture, and modern visual styling), the BOS C++ UI framework provides an attractive aesthetic. However, an architectural audit of the application developer experience demonstrates that developing an application currently requires manual, low-level pixel manipulation:

1. **Pervasive Manual Pixel Coordinate Specification:**
   Application developers must explicitly define bounding boxes (`set_bounds(Rect(x, y, w, h))`), calculate vertical and horizontal offsets manually (`y = prev_y + h + 12`), and hardcode panel dimensions (`set_bounds(Rect(0, 0, 320, 380))`).
2. **Missing or Incomplete Content-Driven Preferred Size Measurement:**
   Although `Widget::measure_preferred_size()` was introduced in the base class, it defaults to returning `m_bounds.size()`. Concrete controls such as `Button`, `TextBox`, `CheckBox`, `Toggle`, `ProgressBar`, and `Card` either do not override `measure_preferred_size()` or return hardcoded stub dimensions (e.g. `Card::measure_preferred_size()` returns a static `240x140`). True content measurement based on font metrics, text width, and icon dimensions is absent in controls.
3. **Control Stretching vs Natural Sizing ("Small Controls Stay Small"):**
   In `LinearLayout::apply()`, cross-axis alignment defaults to `Alignment::Stretch`. When a container uses stretch alignment, all child controls—including buttons and checkboxes—expand to fill the container's entire width, producing unnaturally elongated buttons (e.g. `[ OK ]` stretching to 300px wide).
4. **Lack of High-Level Auto-Framing Containers (`SmartPanel`):**
   Developers must manually instantiate layouts (`new LinearLayout(...)`), manually assign padding insets, and manually coordinate parent and child sizes. There is no high-level container that automatically sizes itself to fit its aggregated contents (`AutoSize = content + padding + spacing`).
5. **Rectangular Window Geometry & Inaccurate Hit-Testing:**
   While buttons and cards feature rounded corners, the actual native OS application window itself is rendered with sharp 90-degree corners in both `userspace/apps/desktop_shell/main.c` (`draw_window_frame`) and `sdk/src/bos/ui/window.cpp`. Furthermore, mouse hit-testing treats the entire bounding box `[x, y, w, h]` as the window area without rejecting clicks in the outside-corner regions.

---

## 2. Forensic Evidence & Root Cause Analysis

### 2.1. Audit of `Widget::measure_preferred_size()` & Control Metrics

- **Location:** `sdk/include/bos/widget.hpp:117`, `sdk/src/bos/ui/widget.cpp:268-270`
- **Current Code:**
  ```cpp
  Size Widget::measure_preferred_size() const {
      return m_bounds.size();
  }
  ```
- **Evidence:**
  `Widget::measure_preferred_size()` returns the current arbitrary bounds rather than querying child content.
- **Concrete Control Implementations:**
  - `Button` (`sdk/src/bos/ui/controls/button.cpp`): Does not override `measure_preferred_size()`. Defaults to initial `(120, 32)`.
  - `Label` (`sdk/src/bos/ui/controls/label.cpp`): Does not override `measure_preferred_size()`. Defaults to initial `(100, 20)`.
  - `TextBox` (`sdk/src/bos/ui/controls/textbox.cpp`): Does not override `measure_preferred_size()`. Defaults to initial `(180, 32)`.
  - `CheckBox` (`sdk/src/bos/ui/controls/checkbox.cpp`): Does not override `measure_preferred_size()`. Defaults to initial `(120, 24)`.
  - `Toggle` (`sdk/src/bos/ui/controls/toggle.cpp`): Does not override `measure_preferred_size()`. Defaults to initial `(44, 22)`.
  - `Card` (`sdk/src/bos/ui/controls/card.cpp:72`): Returns static `Size(240, 140)`. Does not aggregate child widgets or compute header extent.

**Root Cause:**
Because controls do not report their true content dimensions, layout managers cannot compute dynamic natural bounds. Application authors are forced to hardcode explicit dimensions.

---

### 2.2. Audit of Container Layouts (`LinearLayout`, `AnchorLayout`)

- **Location:** `sdk/include/bos/layout.hpp:51-145`
- **Evidence:**
  - In `LinearLayout::apply()`, if `m_alignment == Alignment::Stretch`, every child width is set to `content.width - m.horizontal()`. Small controls (like buttons) cannot opt out of stretching unless individual controls define alignment override policies or the container supports preferred sizing with alignment preservation.
  - When `m_alignment != Alignment::Stretch`, `pref.width` is used, but because `pref.width` defaults to `m_bounds.width`, the layout cannot adapt to dynamic text changes.
  - Sizing modes (`AutoWidth`, `AutoHeight`, `Fill`, `Fixed`) do not exist on containers. The container dimensions are completely passive: containers do not adjust their own bounds to enclose their children.

**Root Cause:**
The layout system is purely top-down (parent imposes bounds on children) rather than two-pass (bottom-up measure preferred sizes, then top-down arrange).

---

### 2.3. Audit of Window Rounding & Hit Testing

- **Location:** `userspace/apps/desktop_shell/main.c:1264-1292` & `2130-2158`, `sdk/src/bos/ui/window.cpp:230-252` & `550-570`
- **Evidence:**
  - `desktop_shell/main.c:1272`:
    ```c
    fill_rect(fb, stride, x, y, w, h, col_border);
    fill_rect(fb, stride, x + 1, y + 1, w - 2, 31, col_titlebar);
    ...
    fill_rect(fb, stride, x + 1, y + 32, w - 2, h - 33, col_body);
    ```
    The window boundary is strictly rectangular.
  - `desktop_shell/main.c:2142-2157`:
    ```c
    if (mx >= wx && mx < wx + ww && my >= wy && my < wy + 32) { ... dragging = true; }
    if (mx >= wx && mx < wx + ww && my >= wy + 32 && my < wy + wh) { handle_window_client_click(...); }
    ```
    If a user clicks within the bounding box corners (e.g. `(wx + 2, wy + 2)`), it is registered as a window click even though visually it should fall in the rounded corner dead-zone.
  - `sdk/src/bos/ui/window.cpp`: Window client/non-client areas and surface clears are square rectangles.

**Root Cause:**
The window frame drawing was implemented with `fill_rect` rather than the anti-aliased rounded rectangle geometry available in the OS (`fill_rounded_rect_fast`), and corner distance checking is absent from the mouse dispatch routines.

---

### 2.4. Audit of Developer Ergonomics in Showcase Applications

- **Location:** `userspace/apps/ui_behavior_demo/main.cpp`
- **Evidence:**
  - Constructing a single card panel requires 8 lines of boilerplate:
    ```cpp
    m_left_panel = new bos::Card("Controls & Focus Navigation");
    m_left_panel->set_subtitle("Tab and Shift+Tab accessible keyboard traversal");
    m_left_panel->set_bounds(bos::Rect(0, 0, 320, 380)); // MANUAL PIXEL COORDINATES
    m_left_layout = new bos::LinearLayout(bos::Orientation::Vertical, 10, bos::Alignment::Stretch);
    m_left_panel->set_layout(m_left_layout);
    m_left_panel->set_padding(bos::Insets(12, 40, 12, 12)); // MANUAL PADDING CONSTANTS
    ```
  - Adding controls requires manual margin calculations or accepts forced stretching.
  - Dynamic removal or addition of children does not trigger automatic panel reflow or resizing.

---

## 3. Files Involved

| File | Subsystem | Responsibility |
|------|-----------|----------------|
| `sdk/include/bos/theme.hpp` | Design System | Add `RadiusWindow = 10` token and layout metrics. |
| `sdk/src/bos/ui/theme.cpp` | Design System | Implement design token definitions. |
| `sdk/include/bos/widget.hpp` | UI Core | Enhance `measure_preferred_size()`, `preferred_size()`, and layout invalidation. |
| `sdk/src/bos/ui/widget.cpp` | UI Core | Base layout invalidation and coordinate transforms. |
| `sdk/include/bos/layout.hpp` | Layout System | Alignments, stretch policies, and measurement queries. |
| `sdk/include/bos/controls/smart_panel.hpp` | Containers (NEW) | High-level auto-framing smart panel container with sizing modes. |
| `sdk/src/bos/ui/controls/smart_panel.cpp` | Containers (NEW) | Implementation of `SmartPanel` auto-sizing and child framing. |
| `sdk/include/bos/controls/button.hpp` | Controls | Button sizing policy and preferred size override. |
| `sdk/src/bos/ui/controls/button.cpp` | Controls | Content-driven width measurement (text metrics + icon + padding). |
| `sdk/include/bos/controls/label.hpp` | Controls | Preferred size override. |
| `sdk/src/bos/ui/controls/label.cpp` | Controls | Text measurement via `Font::measure()`. |
| `sdk/include/bos/controls/textbox.hpp` | Controls | Preferred size override. |
| `sdk/src/bos/ui/controls/textbox.cpp` | Controls | Input width/height metrics. |
| `sdk/include/bos/controls/checkbox.hpp` | Controls | Preferred size override. |
| `sdk/src/bos/ui/controls/checkbox.cpp` | Controls | Indicator + label metrics. |
| `sdk/include/bos/controls/toggle.hpp` | Controls | Preferred size override. |
| `sdk/src/bos/ui/controls/toggle.cpp` | Controls | Pill toggle metrics. |
| `sdk/include/bos/controls/progressbar.hpp` | Controls | Preferred size override. |
| `sdk/src/bos/ui/controls/progressbar.cpp` | Controls | Progress bar metrics. |
| `sdk/include/bos/controls/card.hpp` | Controls | Preferred size override. |
| `sdk/src/bos/ui/controls/card.cpp` | Controls | Aggregated content + header measurement. |
| `sdk/include/bos/window.hpp` | Window System | Window corner rounding support and corner hit testing. |
| `sdk/src/bos/ui/window.cpp` | Window System | Non-client rounded presentation and outside corner click rejection. |
| `userspace/apps/desktop_shell/main.c` | Desktop Shell | Real rounded window frame rendering (`RadiusWindow = 10`) and corner hit testing. |
| `userspace/apps/smart_layout_demo/main.cpp` | Application (NEW) | Usability demonstration app built with smart layout primitives and zero manual coordinates. |
| `build.ps1` | Build Engine | Compile `smart_panel.cpp` into `libbos_ui_cpp.a` and link `smart_layout_demo.elf`. |

---

## 4. Risk Analysis

| Risk | Severity | Mitigation Strategy |
|------|----------|---------------------|
| Layout recursion storms during auto-sizing | High | Cache measured preferred sizes; only recalculate when layout is marked dirty (`m_layout_dirty`). Prevent re-entrant layout calls during `set_bounds`. |
| Breaking existing Phase 1–4 applications | Critical | Maintain backward compatibility for `Widget`, `Window`, `LinearLayout`, `AnchorLayout`, and existing controls. New functionality extends classes cleanly. |
| Window corner hit-test discrepancies | High | Implement identical geometric corner rejection math in both `desktop_shell/main.c` and `sdk/src/bos/ui/window.cpp` using the formula $(mx - cx)^2 + (my - cy)^2 > R^2$. |
| Visual artifacts during window resize | Medium | Ensure complete surface clearing and redrawing with proper clipping; avoid drawing stale rectangular background pixels outside the rounded boundary. |
| Bloating developer API surface | Medium | Keep `SmartPanel` API concise and fluent: `add(widget)`, `set_sizing_mode(...)`, `set_orientation(...)`. Defaults directly sourced from `Theme`. |

---

## 5. Suspected Fix & Recommendation

1. Implement dynamic preferred size measurement on all fundamental controls using `Font::measure()` and `Theme` metrics tokens.
2. Introduce `SmartPanel` (`sdk/include/bos/controls/smart_panel.hpp` and `sdk/src/bos/ui/controls/smart_panel.cpp`) supporting `AutoWidth`, `AutoHeight`, `AutoSize`, `Fill`, and `Fixed` modes with centralized design token spacing and padding.
3. Update `LinearLayout` to respect preferred sizing and keep small controls naturally sized without unwanted stretching.
4. Implement true rounded window rendering and corner hit-test rejection (`RadiusWindow = 10`) in both `desktop_shell/main.c` and `sdk/src/bos/ui/window.cpp`.
5. Build `smart_layout_demo` demonstrating an elegant responsive application with near-zero manual pixel positioning.

---
**Verdict:** Forensic investigation complete. Proceeding to Task 2 (Architecture & Patch Plan). NO SOURCE CODE MODIFIED.
