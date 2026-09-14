# PATCH PLAN — BOS C++ UI FRAMEWORK PHASE 2
## Modern Visual Controls & PNG / Image Engine

**Target:** ATOMS OS Native C++ UI Framework  
**Scope:** Phase 2 Visual Controls, PNG/Deflate Decoding Engine, Visual State Architecture, 9-Slice Renderer, Showcase App  
**Input:** `docs/cplusplus_ui/PHASE2_FORENSIC_REPORT.md`  
**Date:** 2026-09-10  
**Phase:** TASK 2 — ARCHITECT TEAM (PLANNING ONLY — NO SOURCE CODE MODIFIED)

---

## 1. Architectural Strategy & Extension Model

Phase 2 builds directly on the locked Phase 1 foundation (`Application`, `Window`, `Surface`, `Widget`, `Event`, `Layout`, `Resource`, `Types`, `Geometry`).
- Base `Widget` remains the root polymorphic primitive for all controls.
- `Surface` is augmented with hardware-accelerated/optimized software alpha blending (`blend_pixel`), scaled image rendering (`draw_image`), and 9-slice scalable card/border rendering (`draw_image_9slice`).
- The PNG engine implements native RFC 1951 DEFLATE decompression and chunk parsing without external libraries.
- The control hierarchy is modularized into `sdk/include/bos/controls/` and `sdk/src/bos/ui/controls/`.
- The showcase application demonstrates an ATOMS Settings & Personalization dashboard directly inspired by the design specification.

---

## 2. File Modification & Creation Inventory

### 2.1 Existing Files to Extend (Non-Breaking Extensions Only)
1. **`sdk/include/bos/resource.hpp`**:
   - Add RAII deallocation in `Bitmap` (free on destruct if `owns_memory`).
   - Add `Image::from_file(const char* path)`, `Image::from_memory(const uint8_t* data, size_t size)`, `Image::create_solid(uint32_t w, uint32_t h, Color color)`.
2. **`sdk/include/bos/surface.hpp`**:
   - Add `blend_pixel(int32_t x, int32_t y, Color color)`.
   - Add `draw_image(const Image& image, const Rect& dest, const Rect* src = nullptr)`.
   - Add `draw_image_9slice(const Image& image, const Rect& dest, const Insets& borders)`.
   - Add `fill_rounded_rect(const Rect& rect, uint32_t radius, Color color)`.
   - Add `draw_rounded_rect(const Rect& rect, uint32_t radius, Color color, uint32_t thickness = 1)`.
3. **`sdk/src/bos/ui/surface.cpp`**:
   - Implement alpha compositing and 9-slice mathematics:
     - Corners (A, C, G, I) copied without stretching.
     - Borders (B, D, F, H) stretched along their axes.
     - Center (E) scaled to fill remaining inner bounds.
4. **`sdk/include/bos/ui.hpp`**:
   - Include all Phase 2 headers: `png.hpp`, `state.hpp`, `icon.hpp`, `scale.hpp`, `theme.hpp`, and all controls.
5. **`build.ps1`**:
   - Add compilation steps for all new `.cpp` files into `build/libbos_ui_cpp.a`.
   - Add target `build/settings_demo.elf`.

### 2.2 New Subsystems to Create
1. **`sdk/include/bos/png.hpp` & `sdk/src/bos/ui/png.cpp`**:
   - Standalone PNG chunk parser (`IHDR`, `IDAT`, `PLTE`, `IEND`).
   - RFC 1951 DEFLATE decompressor (supporting uncompressed, fixed, and dynamic Huffman blocks).
   - Scanline unfiltering (Paeth, Up, Sub, Average).
   - Conversions for RGBA, RGB, Grayscale, Grayscale+Alpha, and Indexed palette into 32-bpp ARGB buffer.
2. **`sdk/include/bos/state.hpp`**:
   - `enum class VisualState { Normal, Hover, Pressed, Focused, Disabled, Selected };`
   - `struct StateStyle`: background, border, text color, corner radius.
   - `struct ControlStyle`: multi-state style resolver with fallback rules.
3. **`sdk/include/bos/icon.hpp`**:
   - `Icon` class holding image, target size (e.g. 16, 24, 32, 48), opacity, and tint.
   - Helper methods to render aligned next to text (`IconPosition::Left`, `IconPosition::Right`).
4. **`sdk/include/bos/scale.hpp`**:
   - DPI scale management: `Scale::to_physical()`, `Scale::to_logical()`.
   - Pre-calculated scale factors (100%, 125%, 150%, 175%, 200%).
5. **`sdk/include/bos/theme.hpp`**:
   - Consistent modern styling: Slate palette, Blue accent, Emerald success, Amber warning.
   - Default typography metrics, button heights, padding tokens, radii.

### 2.3 New Controls (`sdk/include/bos/controls/` & `sdk/src/bos/ui/controls/`)
1. **`Button`**: Full visual state model (Normal, Hover, Pressed, Focused, Disabled), text + icon alignment, 9-slice or flat rounded rect.
2. **`Label`**: Monospace font, color, alignment, disabled dimming, link style option.
3. **`TextBox`**: Active focus border, placeholder text, keyboard character input, backspace, cursor positioning, read-only flag.
4. **`CheckBox`**: Checkmark glyph rendering, checked state toggle, click & spacebar toggling.
5. **`RadioButton`**: Circle glyph with inner bullet, mutual exclusion group ID management.
6. **`Toggle`**: Pill-shaped switch with sliding thumb, smooth on/off visual indication.
7. **`ListView` / `List`**: Item collection, hover item highlight, selected item tracking, item click callback.
8. **`ComboBox`**: Collapsed display showing selected value + dropdown chevron, interactive dropdown list, selection commit.
9. **`ProgressBar`**: Min/max/value range, filled track, percentage text overlay.
10. **`Card`**: Container widget with rounded corners, border, elevation color, optional header and child layout.
11. **`Sidebar`**: Left navigation strip with icons, labels, active section indicator, selection change events.
12. **`ScrollView`**: Viewport bounds, scroll offset, vertical scrolling, clipping, scrollbar thumb rendering.
13. **`TabControl`**: Horizontal tab headers, active tab switching, display of selected child page.
14. **`ImageWidget`**: Display widget for `bos::Image` with fit modes: `Fit`, `Fill`, `Center`, `Stretch`.

### 2.4 Showcase Application
- **`userspace/apps/settings_demo/main.cpp`**:
  - Full modern desktop Settings & Personalization application matching the visual reference:
    - Sidebar navigation with icons (`Home`, `System`, `Bluetooth`, `Network`, `Personalization`, `Apps`, `Accounts`).
    - Breadcrumb navigation: `Personalization > Themes`.
    - High-quality wallpaper/theme preview card.
    - Theme selection cards.
    - Control showcase panel: Buttons in 4 states, CheckBox, RadioButtons, Toggle, Label, TextBox, ComboBox, ListView, ProgressBar, ScrollView, Card, Tabs.
    - Interactive DPI scaling switcher (100%, 150%, 200%).

---

## 3. Rollback & Contingency Plan

If any new control or the PNG decoder encounters compilation or linking errors:
1. All new controls are isolated in `sdk/include/bos/controls/` and `sdk/src/bos/ui/controls/`.
2. Static library `libbos_ui_cpp.a` can be rolled back to Phase 1 object list with zero impact on existing C apps (`sdk_explorer.elf`, `gui_demo.elf`).
3. Phase 1 demo (`cpp_ui_demo.elf`) remains functional and will be verified as a regression baseline throughout.
