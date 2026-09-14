# ATOMS OS / BOS C++ UI FRAMEWORK
## PHASE 4A — ARCHITECTURE PATCH PLAN (TASK 2)

---

### 1. Scope & Objective
Phase 4A establishes the formal BOS visual design system: centralized design tokens (metrics, radii, surfaces, elevation, color tokens), real anti-aliased rounded corner geometry, an open-source typography engine (Inter under SIL Open Font License 1.1), and cohesive state styles for all controls.

---

### 2. Detailed Modifications & Rationale

#### Component 1: Open-Source Font Asset & Licensing
- **File:** `third_party/fonts/inter/OFL.txt` (NEW)
- **File:** `third_party/fonts/inter/Inter-Regular.ttf` & `Inter-Bold.ttf` (NEW)
- **File:** `third_party/fonts/inter/inter_font_assets.hpp` / `.cpp` (NEW)
- **Why:** Replaces proprietary Windows Segoe UI with legally redistributable, SIL OFL 1.1 licensed Inter font. Contains complete uppercase A-Z, lowercase a-z, digits 0-9, and standard punctuation. Pre-rasterized into compact freestanding C++ glyph tables with alpha coverage and metrics.
- **Expected Result:** Zero legal ambiguity, zero external internet dependencies at runtime, 100% offline freestanding build.

#### Component 2: C++ Typography Subsystem
- **File:** `sdk/include/bos/font.hpp` (NEW)
- **File:** `sdk/src/bos/ui/font.cpp` (NEW)
- **File:** `sdk/include/bos/surface.hpp` & `sdk/src/bos/ui/surface.cpp` (MODIFY)
- **Why:** Introduces `Font`, `FontFace`, `FontWeight` (`Regular`, `Medium`, `Bold`), `TextMetrics` (`width`, `height`, `line_count`, `baseline`), and `TextAlignment` (`Left`, `Center`, `Right`).
- **Expected Result:** Allows controls and applications to draw high-definition, proportional text with proper measurement, vertical alignment, and ellipsis clipping without memory allocations.

#### Component 3: Centralized Design Tokens & Theme Subsystem
- **File:** `sdk/include/bos/theme.hpp` & `sdk/src/bos/ui/theme.cpp` (MODIFY)
- **Why:** Centralizes all visual properties into immutable design tokens:
  - **Corner Radii Scale:** `RadiusSmall()` (4px), `RadiusControl()` (6px), `RadiusCard()` (10px), `RadiusPill()` (16-20px).
  - **Spacing Scale:** `SpacingSmall()` (4px/6px), `SpacingMedium()` (12px), `SpacingLarge()` (20px), `SpacingXLarge()` (28px).
  - **Metrics Scale:** `ButtonHeight()` (32px), `InputHeight()` (32px), `HeaderHeight()` (44px), `ControlPadding()`.
  - **Surface Elevation:** `Background()`, `Surface()`, `SurfaceElevated()`, `SurfaceSubtle()`, `Divider()`.
  - **Colors:** Cohesive Dark Slate and Light Pearl palettes with single Royal Blue (`#2563EB`) accent and Emerald (`#10B981`) success state.
  - **Button Variants:** `make_primary_button_style()`, `make_secondary_button_style()`, `make_ghost_button_style()`.
- **Expected Result:** Controls no longer draw ad-hoc boxes; every surface and border derives systematically from the active theme.

#### Component 4: Control Redesign (Real Rounded Geometry & States)
- **File:** `sdk/include/bos/controls/button.hpp` & `sdk/src/bos/ui/controls/button.cpp` (MODIFY)
  - Distinguish Primary vs Secondary variants; apply `RadiusControl()`; render smooth states (Normal, Hover, Pressed, Focused, Disabled) and non-overlapping 2px focus ring.
- **File:** `sdk/include/bos/controls/textbox.hpp` & `sdk/src/bos/ui/controls/textbox.cpp` (MODIFY)
  - Real rounded corners, proportional font rendering, subtle placeholder, active blinking caret, and 2px focus ring.
- **File:** `sdk/include/bos/controls/card.hpp` & `sdk/src/bos/ui/controls/card.cpp` (MODIFY)
  - Surface elevation, subtle 1px border, typography-driven header with Inter bold title and secondary subtitle.
- **File:** `sdk/include/bos/controls/toggle.hpp` & `progressbar.cpp` (MODIFY)
  - Ensure true pill geometry with zero square clipping artifacts.

#### Component 5: Showcase Integration
- **File:** `userspace/apps/ui_behavior_demo/main.cpp` (MODIFY)
  - Showcase Primary Accent Button, Secondary Neutral Button, Disabled Button, Rounded Textbox with caret, Segmented Theme Switch pill, and Inter typography hierarchy.
  - Maintain 100% of all 12 certified tests (Tests A through L).
- **File:** `userspace/apps/desktop_shell/main.c` (MODIFY)
  - Synchronize embedded desktop showcase window with new typography, rounded geometry, and segmented pill control.

---

### 3. Risk & Mitigation Matrix

| Risk | Likelihood | Impact | Mitigation Strategy |
| :--- | :--- | :--- | :--- |
| Freestanding memory allocation during text layout | Low | High | Pre-calculate glyph metrics at build time; `measure_string` and `draw_string` perform zero dynamic heap allocations. |
| Binary size expansion from embedded fonts | Low | Medium | Generate compact, optimized glyph tables covering ASCII 32..126 + essential Latin symbols (< 60 KB per weight). |
| Performance overhead from rounded rect loops | Low | Low | Optimized bounding box intersection ensures pixel loops only test the 4 corner arc sub-rectangles. |
| Regression of Phase 4 interaction certifications | Low | Critical | Automated certification suite in `ui_behavior_demo` runs immediately after compilation to verify 12/12 PASS. |

---

### 4. Rollback Plan
All modifications are strictly additive to the C++ UI SDK and showcase applications. If any unexpected compilation or runtime issue arises:
1. Revert `sdk/` changes via `git checkout sdk/`.
2. Re-link `libbos_ui_cpp.a` and `desktop_shell.elf`.
3. Verify previous stable binaries.
