# ATOMS OS — PHASE 4 UI SHOWCASE VISUAL POLISH & REDESIGN
## ARCHITECTURE & PATCH PLAN (TASK 2)

### 1. Architectural Objective
Redesign the Phase 4 UI Showcase application ("UI Test") inside `userspace/apps/desktop_shell/main.c` and polish `userspace/apps/ui_behavior_demo/main.cpp` into a state-of-the-art native BOS application.

---

### 2. Files to Modify

| File | Subsystem | Action |
|---|---|---|
| `userspace/apps/desktop_shell/main.c` | Desktop Shell & Window Presenter | Visual redesign of `draw_window_frame()`, `render_ui_test_window()`, and coordinate update in `handle_window_client_click()` |
| `userspace/apps/ui_behavior_demo/main.cpp` | Standalone C++ Phase 4 Application | Visual typography & header polish in cards while preserving 12 certification tests |

---

### 3. Detailed Component Plan (NO CODE)

#### Component A: Typography Engine Extension (`desktop_shell/main.c`)
- Add `draw_char_scaled()` and `draw_string_scaled()`: 2x integer scale multiplier yielding 12x16px crisp glyphs for page titles.
- Add `draw_char_bold()` and `draw_string_bold()`: 1px horizontal font thickening for section headings.
- Add `draw_focus_ring()`: Consistent 2px high-contrast focus indicator with 2px padding around controls.

#### Component B: Window Chrome Redesign (`desktop_shell/main.c`)
- Modify `draw_window_frame()`:
  - Outer border: Replaced from `0xFF38BDF8` (harsh cyan) to `0xFF334155` (Slate-700 dark) or `0xFFCBD5E1` (Slate-300 light).
  - Title Bar: Replaced from flat grey to deep charcoal `0xFF151D2A` with 1px bottom accent line.
  - App Identity: Add a 16x16 app badge at top-left.
  - Close Button: Refined 20x20 rounded control with centered 'x', avoiding aggressive neon red blocks.
  - Application Title: Set to `"UI Behavior Showcase"` for clean professional presentation.

#### Component C: Sidebar Navigation Redesign (`desktop_shell/main.c`)
- Width: 190px.
- Background: Deep obsidian `0xFF0B101B` (dark) / `0xFFF1F5F9` (light) with 1px vertical divider.
- Add category caption: `"NAVIGATION"`.
- Tab items:
  - Tab 0: `"Controls & UI"`
  - Tab 1: `"Diagnostics"`
  - Tab 2: `"Architecture"`
- Selected tab: Subtle pill background (`0xFF1E293B`), 4px royal blue left accent bar (`0xFF3B82F6`), crisp white text (`0xFFFFFFFF`). No neon cyan flood fills.
- Inactive tabs: Muted slate text (`0xFF94A3B8`).

#### Component D: Interactive Page Redesign (`desktop_shell/main.c` - Tab 0)
- **Header:** 2x scaled title `"Controls & Experience"`, muted subtitle, 1px horizontal separator.
- **Section 1 (Appearance):** Natural section header `"Appearance"`, subtitle `"Select application theme mode"`. Segmented pill switch with `"Dark Slate"` and `"Light Pearl"` options.
- **Section 2 (Interaction & Gestures):** Dual-column layout with two clean surface cards (`0xFF1E293B`):
  - Left: `"Primary Action"` — Button `"Click Me  (Clicks: N)"` in royal blue (`0xFF2563EB`).
  - Right: `"Drag-Off Cancellation"` — Button `"Hold & Drag Pointer Out"` with safety status feedback (`Confirmed` / `Safety Cancelled (Pass)`).
- **Section 3 (Motion Engine):** Modern motion card with sleek progress track, royal blue fill, percentage indicator, and `[ Pause ]` / `[ Reset ]` secondary action buttons.
- **Section 4 (Display & Accessibility):** Inline controls for `Hardware VSync (60 Hz)` checkbox and `Dirty-Rect Compositing` pill toggle switch.
- **Footer:** Sleek `[ Reset All ]` button and keyboard navigation guidance.
- **Remove:** All "1.", "2.", "3." numbered prefixes and all nested cyan borders.

#### Component E: Event Hit-Testing Synchronization (`desktop_shell/main.c`)
- Align bounding boxes in `handle_window_client_click()` and `BOS_GUI_EVENT_MOUSE_UP` to match exact rendered control positions.
- Preserve Tab key cycling (controls 0..5), Enter/Space activation, drag-off release logic, and idle 60fps easing animation.

#### Component F: Diagnostics & Architecture Pages (`desktop_shell/main.c` - Tabs 1 & 2)
- Tab 1: Clean table with uppercase headers (`SUBSYSTEM`, `STATUS`, `LATENCY`), 12 test rows with calm emerald green `Pass` badges, and an operational summary banner.
- Tab 2: Structured key/value specification cards for Runtime Engine, Window Engine & Compositor, and Certified Binary Modules.

---

### 4. Expected Results
- Zero visual noise: Eliminates neon cyan borders, all-caps text, and test-harness appearance.
- Clear visual hierarchy with crisp scaled typography and generous whitespace.
- 100% functional equivalence with existing certified Phase 4 test behavior.

---

### 5. Risk & Rollback Plan
- **Risk:** Hit-test mismatch if click coordinates do not mirror render formulas.
- **Mitigation:** Use shared position offset variables (`cx`, `cy`, `cw`) across both rendering and click handling.
- **Rollback:** `git checkout userspace/apps/desktop_shell/main.c userspace/apps/ui_behavior_demo/main.cpp`.
