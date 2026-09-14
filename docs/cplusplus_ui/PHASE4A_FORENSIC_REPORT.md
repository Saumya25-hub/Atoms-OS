# ATOMS OS / BOS C++ UI FRAMEWORK
## PHASE 4A — FORENSIC REPORT (TASK 1)

---

### 1. Executive Summary & Objective
Phase 4 successfully delivered runtime event dispatch, mouse capture, keyboard focus traversal, responsive layout reflow, dynamic theme switching, 60 FPS easing animations, hierarchical clipping, and dirty region damage invalidation. While an initial layout polish pass improved visual grouping, the framework still lacks a formal, native BOS visual language and design token architecture.

Controls are rendered ad-hoc (often thinking "draw a button" or "draw a box" instead of resolving through a centralized design token system), geometry remains visibly square/rectangular with crude corners, typography relies on a legacy 8x16 monospace bitmap font without weight/metrics abstractions, and open-source interface typography is missing.

Phase 4A establishes the formal BOS visual design system: centralized design tokens (metrics, radii, surfaces, elevation, color tokens), real anti-aliased rounded corner geometry, an open-source typography engine (Inter under SIL Open Font License 1.1), and cohesive state styles for all controls.

---

### 2. Forensic Findings & Root Cause Analysis

#### Finding 1: Primitive and Incomplete Rounded Corner Geometry
- **Evidence:** `Surface::fill_rounded_rect()` and `Surface::draw_rounded_rect()` exist in `sdk/src/bos/ui/surface.cpp` using basic integer distance approximations. However:
  - Many controls (like `TextBox`, `Button`, `ListView`, and headers) do not systematically consume a shared radius token hierarchy (`RadiusSmall`, `RadiusControl`, `RadiusCard`, `RadiusPill`).
  - Child clipping in containers (`ScrollView`, `Card`) uses raw axis-aligned bounding rectangles (`Rect`), causing inner content or focus borders to visually puncture rounded container corners.
  - Controls often render sharp rectangular outlines, causing the overall OS interface to look rigid and blocky on physical displays.

#### Finding 2: Lack of Typography Engine & Open-Source Font Assets
- **Evidence:** `Surface::draw_string()` in `sdk/src/bos/ui/surface.cpp` is hardcoded to `g_font8x16_stub` (an 8x16 monospace bitmap font).
  - No typography abstraction exists in the C++ UI SDK (`Font`, `FontFace`, `FontWeight`, `TextMetrics`).
  - Text cannot be rendered with semantic weights (Regular, Medium, SemiBold/Bold), line heights, baselines, alignments (`Left`, `Center`, `Right`), or clipping with ellipsis (`...`).
  - The repository previously attempted font atlas generation (`tools/generate_bofont.py`) using proprietary Windows system fonts (`segoeui.ttf`), which violates licensing requirements.
  - An authentic, legally redistributable open-source UI font (Inter under SIL Open Font License 1.1) must be integrated into `third_party/fonts/inter/` and compiled into offline freestanding font assets.

#### Finding 3: Inconsistent Button Visual Model & State Styles
- **Evidence:** `Button::paint()` in `sdk/src/bos/ui/controls/button.cpp` resolves state styles, but:
  - There is no distinction between Primary Action buttons (accented, solid, high-priority), Secondary Neutral buttons (subtle surface, calm contrast), and Ghost/Tertiary buttons.
  - State transitions between Normal, Hover, Pressed, Focused, and Disabled lack standardized visual tokens for surface elevation, border opacity, and text contrast.
  - Focus rings currently draw 2px borders that directly touch or overwrite button text or edge pixels rather than offset focus rings.

#### Finding 4: Inadequate Surface Elevation & Hierarchy System
- **Evidence:** `Theme` in `sdk/src/bos/ui/theme.cpp` defines a basic set of colors, but lacks semantic surfaces:
  - Missing defined tokens for `SurfaceElevated`, `SurfaceSubtle`, `SurfaceOverlay`, and `Divider`.
  - Interfaces tend to rely on 1px/2px borders for separation rather than surface contrast and whitespace.
  - Both Dark Slate and Light Pearl themes need refined palettes ensuring equal visual quality and readability.

#### Finding 5: Decentralized Visual Metrics
- **Evidence:** Various controls and UI pages hardcode pixel heights (e.g. `20`, `26`, `32`, `38`), internal paddings (`4`, `8`, `12`), and spacings without deriving them from a centralized `Theme::Metrics` structure scaled via `Scale::to_physical()`.

---

### 3. Files Involved in Phase 4A

1. **Design System & Tokens:**
   - `sdk/include/bos/theme.hpp`
   - `sdk/src/bos/ui/theme.cpp`
   - `sdk/include/bos/types.hpp`
   - `sdk/include/bos/scale.hpp`

2. **Typography Subsystem:**
   - `sdk/include/bos/font.hpp` (NEW: C++ Font, FontFace, FontWeight, TextMetrics, TextAlignment)
   - `sdk/src/bos/ui/font.cpp` (NEW: Font loader, glyph lookup, string metrics, text alignment)
   - `sdk/include/bos/surface.hpp`
   - `sdk/src/bos/ui/surface.cpp` (Anti-aliased/alpha-blended rounded rects, font-aware text rendering, ellipsis clipping)
   - `third_party/fonts/inter/` (NEW: Inter font assets + SIL Open Font License `OFL.txt`)
   - `tools/generate_bofont.py` (Updated to consume licensed Inter font instead of Windows Segoe)

3. **Core Controls Enhancement:**
   - `sdk/include/bos/controls/button.hpp` & `sdk/src/bos/ui/controls/button.cpp` (Primary/Secondary/Ghost styles, rounded radii, offset focus ring)
   - `sdk/include/bos/controls/textbox.hpp` & `sdk/src/bos/ui/controls/textbox.cpp` (Rounded input styling, caret, placeholder, font integration)
   - `sdk/include/bos/controls/card.hpp` & `sdk/src/bos/ui/controls/card.cpp` (Surface elevations, typography hierarchy)
   - `sdk/include/bos/controls/toggle.hpp` & `sdk/src/bos/ui/controls/toggle.cpp` (Smooth pill geometry)
   - `sdk/include/bos/controls/progressbar.hpp` & `sdk/src/bos/ui/controls/progressbar.cpp` (Rounded pill track & indicator)
   - `sdk/include/bos/controls/scrollview.hpp` & `sdk/src/bos/ui/controls/scrollview.cpp` (Rounded subtle scrollbar thumb)

4. **Showcase Applications:**
   - `userspace/apps/ui_behavior_demo/main.cpp` (Standalone C++ UI framework showcase)
   - `userspace/apps/desktop_shell/main.c` (Desktop shell embedded showcase & window frame)

---

### 4. Risk Analysis

1. **Binary Size Risk:**
   - Embedding multiple full TrueType fonts into freestanding kernel/userspace ELF can bloat binaries.
   - *Mitigation:* Generate compact, optimized C++ glyph atlases (128 ASCII codepoints + essential symbols) for UI Regular, Medium, and Bold weights. Total asset footprint < 60 KB.
2. **Freestanding C++ Runtime Constraints:**
   - The UI framework runs with `-nostdlib -ffreestanding -fno-exceptions -fno-rtti`.
   - *Mitigation:* Typography abstraction must avoid dynamic runtime allocations during text measurement and paint passes, using pre-calculated glyph metrics and static/stack buffers.
3. **Regression Risk on Existing Certified Tests:**
   - Phase 4 certified 12 formal interaction tests (Tests A through L).
   - *Mitigation:* Hit-testing bounds, event handlers, Tab navigation order, drag-off cancellation, and dynamic theme switching APIs must remain 100% backward compatible.

---

### 5. Suspected Fix Strategy (No Code)
- Package the legally vetted Inter font (under SIL OFL 1.1) in `third_party/fonts/inter/`.
- Introduce a freestanding C++ `bos::Font` and `bos::TextMetrics` engine in the SDK.
- Expand `bos::Theme` to provide a complete design token system: metrics scale, corner radii scale, surface elevations, and button variants.
- Upgrade `Surface` rendering primitives for smooth rounded rects and font-aware string drawing.
- Refactor `Button`, `TextBox`, `Card`, and other controls to strictly consume design tokens.
- Update `ui_behavior_demo` and `desktop_shell` to showcase the new visual language.
