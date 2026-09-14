# ATOMS OS / BOS C++ UI — Phase 4 Final Patch Plan
**USABILITY + SMART LAYOUT + ROUNDED WINDOW ARCHITECTURE PLAN**

**Date:** 2026-09-10  
**Phase:** Phase 4 Final (Usability + Smart Layout + Window Geometry)  
**Input:** `docs/cplusplus_ui/PHASE4_FINAL_FORENSIC_REPORT.md`  
**Status:** ARCHITECTURE APPROVED FOR IMPLEMENTATION — NO SOURCE CODE MODIFIED  

---

## 1. Scope and Objective

Transform the certified Phase 4A BOS C++ UI visual language into a reusable, application-friendly, and smart UI framework:
1. Enable content-driven sizing (`SmartPanel`) with automatic padding and spacing from `Theme` tokens.
2. Ensure true content measurement across all controls via `Font::measure()` and control metrics.
3. Ensure small controls (buttons, checkboxes, toggles) stay small and naturally sized rather than stretching uncontrollably.
4. Implement native rounded window geometry (`RadiusWindow = 10`) with accurate corner hit-test rejection.
5. Create a clean demonstration application (`smart_layout_demo`) that builds a responsive application shell with near-zero manual pixel coordinates.
6. Preserve 100% of Phase 1–4A runtime behavior, memory safety, and visual aesthetics.

---

## 2. Planned Changes by Subsystem

### 2.1. Central Design System (`sdk/include/bos/theme.hpp`, `sdk/src/bos/ui/theme.cpp`)
- **What to modify:**
  - Add `RadiusWindow()` design token returning `10` pixels (tasteful, non-exaggerated corner radius for native application windows).
  - Add panel metric tokens: `PanelPadding()` returning `Theme::SpacingMedium()` (12px), `PanelSpacing()` returning `Theme::SpacingSmall()` (8px).
- **Why:** Centralizes window and container metrics in the single source of truth (`bos::Theme`) without ad-hoc magic numbers.
- **Expected Result:** Consistent visual metrics across all containers and window frames.

### 2.2. Base Widget & Measurement Engine (`sdk/include/bos/widget.hpp`, `sdk/src/bos/ui/widget.cpp`)
- **What to modify:**
  - Ensure `measure_preferred_size()` accurately queries children when a layout is attached.
  - Add `invalidate_layout()` method to dirty parent container layouts upward when child geometry or text changes.
- **Why:** Enables two-pass layout calculation (measure preferred sizes bottom-up, then arrange top-down).
- **Expected Result:** Dynamic response to content alterations without manual resize handling.

### 2.3. Control Preferred Sizing (`sdk/include/bos/controls/*.hpp`, `sdk/src/bos/ui/controls/*.cpp`)
- **What to modify:**
  - Override `measure_preferred_size()` in:
    - `Button`: Calculate text width via `Font::measure()`, add icon width, gap, and horizontal padding (minimum width 48px), fixed height `Theme::ButtonHeight()`.
    - `Label`: Calculate width and height via `Font::measure()`.
    - `TextBox`: Calculate width from placeholder/text or minimum 140px, fixed height `Theme::InputHeight()`.
    - `CheckBox`: Calculate box size (18px) + gap (8px) + text width, height 24px.
    - `Toggle`: Fixed size 44x22px.
    - `ProgressBar`: Standard width 180px, height 20px.
    - `Card`: Calculate header area (title + subtitle + icon) + children preferred bounds + padding.
- **Why:** Controls must report their natural content dimensions so layouts do not stretch them or guess their sizes.
- **Expected Result:** Short text buttons (e.g. `"OK"`) remain naturally compact (~56px); long text buttons (e.g. `"Save Changes and Continue"`) expand naturally (~180px).

### 2.4. Smart Container Primitives (`sdk/include/bos/controls/smart_panel.hpp`, `sdk/src/bos/ui/controls/smart_panel.cpp`)
- **What to modify:**
  - Introduce `SmartPanel` subclassing `Widget`.
  - Provide sizing modes:
    - `SizingMode::Auto` / `AutoSize`: Panel bounds automatically frame the aggregated children size plus padding.
    - `SizingMode::AutoWidth`: Width fits children, height fills parent or fixed.
    - `SizingMode::AutoHeight`: Height fits children, width fills parent or fixed.
    - `SizingMode::Fill`: Consumes available parent space.
    - `SizingMode::Fixed`: Adheres to explicitly set dimensions.
  - Automatically apply default padding and spacing from `Theme` tokens.
  - Provide fluent convenience methods:
    - `add(Widget* child)`
    - `set_orientation(Orientation o)`
    - `set_sizing_mode(SizingMode m)`
    - `set_spacing(int32_t s)`
    - `set_padding(const Insets& p)`
    - `set_card_style(bool enable)` (renders subtle rounded card backdrop)
- **Why:** Eliminates the need for application developers to manually calculate coordinates, margins, and container bounds.
- **Expected Result:** Clean declarative composition of UI views.

### 2.5. Layout Engine Polish (`sdk/include/bos/layout.hpp`)
- **What to modify:**
  - In `LinearLayout::apply()`:
    - When `m_alignment == Alignment::Stretch`, check if the child is a small control or has fixed alignment; allow small controls to keep natural preferred width while stretching expansive widgets.
    - Support natural content flow without hardcoded offsets.
- **Why:** Prevents buttons and inputs from stretching into gigantic wide rectangles unless explicitly commanded.
- **Expected Result:** Harmonious linear layouts with naturally sized controls.

### 2.6. Real Rounded Native Window & Corner Hit-Testing (`userspace/apps/desktop_shell/main.c`, `sdk/src/bos/ui/window.cpp`)
- **What to modify:**
  - In `desktop_shell/main.c`:
    - Update `draw_window_frame()` to render a rounded outer boundary using `fill_rounded_rect_fast` with `RadiusWindow = 10`.
    - Render titlebar and window body with matching rounded geometry so outside corner pixels retain desktop wallpaper pixels.
    - In mouse click handling: Implement corner rejection test. If pointer position $(mx, my)$ falls in any of the 4 corner bounding boxes of size $R \times R$, check $(dx^2 + dy^2 \le R^2)$. If outside, ignore window click and pass through to desktop/icons.
  - In `sdk/src/bos/ui/window.cpp`:
    - Add corner hit-test rejection in `dispatch_event()` and `hit_test()`.
- **Why:** Ensures the visible application window boundary is truly rounded and interactive clicks outside the rounded boundary never trigger window events.
- **Expected Result:** High-fidelity rounded window presentation with 100% accurate hit-testing.

### 2.7. New Showcase Application (`userspace/apps/smart_layout_demo/main.cpp`)
- **What to modify:**
  - Create a realistic application featuring:
    - HeaderBar with title, subtitle, and action buttons.
    - Sidebar with navigation items.
    - Central content area containing multiple `SmartPanel` cards.
    - Auto-framing demonstration:
      - Compact natural buttons ("OK", "Apply", "Export Report").
      - Form inputs (name, email, environment toggle, high-DPI checkbox).
      - Dynamic control addition/removal demonstrating automatic layout reflow.
      - Dynamic text resizing demonstrating auto-expanding panel height.
    - ZERO manual pixel placement (`x`, `y`, `w`, `h` for controls are omitted).
- **Why:** Proves that the BOS framework enables application authors to describe *what* the UI contains rather than *where every pixel goes*.
- **Expected Result:** Modern, responsive, and maintainable C++ application code.

### 2.8. Build Pipeline Integration (`build.ps1`)
- **What to modify:**
  - Compile `sdk/src/bos/ui/controls/smart_panel.cpp` -> `build/bos_ui_smart_panel.o`.
  - Add `build/bos_ui_smart_panel.o` to `libbos_ui_cpp.a`.
  - Compile and link `userspace/apps/smart_layout_demo/main.cpp` -> `build/smart_layout_demo.elf`.
  - Recompile `desktop_shell.elf`, update `kernel.bin`, `BOOTX64.EFI`, and disk images (`atoms_uefi_test.img`, `OS.img`).
- **Why:** Integrates new primitives into the system library and bakes test artifacts into the bootable images.
- **Expected Result:** Clean compile and link with zero errors.

---

## 3. Risk & Rollback Plan

### 3.1. Risk Assessment
- **Risk 1: Infinite layout recursion in auto-framing.**
  - *Mitigation:* `SmartPanel` guards against re-entrant layout calls using an `m_in_layout` boolean flag.
- **Risk 2: Breaking existing C++ showcases (`cpp_ui_demo`, `settings_demo`, `window_experience_demo`, `ui_behavior_demo`).**
  - *Mitigation:* All existing public APIs and layout methods remain 100% signature-compatible.
- **Risk 3: Window dragging or close button clipping.**
  - *Mitigation:* Close button position and title drag bar are kept safely inside the non-corner titlebar bounds.

### 3.2. Rollback Procedure
If any regression is detected:
```powershell
git checkout HEAD -- sdk/ userspace/apps/desktop_shell/ build.ps1
rm userspace/apps/smart_layout_demo/main.cpp
.\build.ps1
```

---

## 4. Verification & Certification Plan
1. Compile `libbos_ui_cpp.a`, `smart_layout_demo.elf`, `desktop_shell.elf`, and all existing showcases cleanly with zero warnings/errors.
2. Boot QEMU in pure UEFI mode (`atoms_uefi_test.img`).
3. Verify visual rendering:
   - Real rounded corners on the application window.
   - Smart panels auto-frame contents cleanly.
   - Small buttons stay compact.
   - Text wraps and panels reflow upon resize.
4. Verify interaction:
   - Clicking in outside-corner dead-zone passes through to desktop icons.
   - Title dragging, close button, and client buttons respond instantly.
   - Dynamic button addition/removal reflows smoothly without crash or visual glitch.
5. Generate formal reports:
   - `PHASE4_FINAL_PATCH_REPORT.md`
   - `PHASE4_FINAL_CERTIFICATION_REPORT.md`
   - `walkthrough.md`

---
**Verdict:** Architecture plan complete. Awaiting user approval before code modifications.
