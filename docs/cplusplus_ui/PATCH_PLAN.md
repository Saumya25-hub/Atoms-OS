# PATCH PLAN — BOS C++ UI FOUNDATION IMPLEMENTATION

**Input:** `docs/cplusplus_ui/FORENSIC_REPORT.md`  
**Phase:** TASK 2 — ARCHITECT TEAM (NO CODE MODIFIED)  
**Target:** Implementation of Phase 1 BOS C++ UI Foundation

---

## 1. What to Modify & What to Create

### 1.1 New C++ Framework Headers (`sdk/include/bos/`)
1. `types.hpp`: Color arithmetic (ARGB 32-bit), Result/Error codes, core enums.
2. `geometry.hpp`: Point, Size, Rect, Insets with bounding box arithmetic and clipping.
3. `events.hpp`: Event structures (MouseMove, MouseDown, MouseUp, KeyDown, KeyUp, Char, Close, Resize, Focus).
4. `surface.hpp`: Mapped 2D framebuffer surface abstraction with software rasterizer (clear, fill_rect, draw_rect, draw_line, draw_string).
5. `widget.hpp`: Foundational UI element class with tree hierarchy, bounds, coordinate conversions, and virtual paint/event hooks.
6. `layout.hpp`: Layout interface, `ManualLayout`, and `StackLayout`.
7. `resource.hpp`: Image/asset structures ready for Phase 2 PNG integration.
8. `window.hpp`: RAII native window controller, surface binding, event dispatch, and widget tree management.
9. `application.hpp`: Application lifecycle, window registry, event loop, and graceful exit.
10. `ui.hpp`: Master umbrella header.

### 1.2 New C++ Framework Implementations (`sdk/src/bos/ui/`)
1. `surface.cpp`: Implements 2D rasterization routines and 8x16 bitmap text font.
2. `widget.cpp`: Implements widget tree navigation, coordinate transformations, and dirty-rect invalidation.
3. `window.cpp`: Implements syscall interaction (`SYS_GUI_CREATE_WINDOW`, `SYS_GUI_MAP_SURFACE`, `SYS_GUI_DESTROY_WINDOW`), event routing, and hit-testing.
4. `application.cpp`: Implements window registry, event loop polling (`SYS_GUI_POLL_EVENT`), and CPU yielding (`SYS_YIELD`).

### 1.3 New C++ Demo Application (`userspace/apps/cpp_ui_demo/main.cpp`)
- Real Ring 3 application validating the full stack:
  - Application startup -> Window creation -> Surface mapping -> Widget attachment -> Painting -> Real interactive mouse click/hover & keyboard handling -> Clean shutdown.

### 1.4 Modifications to Existing Build Script (`build.ps1`)
- Add compilation step for `bos_ui_cpp.o` using `clang++`.
- Add compilation and link step for `build/cpp_ui_demo.elf`.
- DO NOT touch or change existing C build steps (`sdk_explorer`, `desktop_shell`, `gui_demo`).

---

## 2. Expected Results

1. `build/cpp_ui_demo.elf` builds cleanly with zero errors and zero warnings.
2. All existing C targets (`build/sdk_explorer.elf`, `build/gui_demo.elf`, `build/desktop_shell.elf`) build cleanly with zero regressions.
3. The framework provides an idiomatic, modern, type-safe C++ API for ATOMS OS applications without runtime overhead or external dependencies.

---

## 3. Rollback Plan

If any unexpected regression or linking incompatibility occurs:
1. Revert changes to `build.ps1`.
2. Delete `userspace/apps/cpp_ui_demo/`.
3. Delete `sdk/src/bos/ui/` and newly added `sdk/include/bos/*.hpp` headers.
4. The existing codebase and C applications remain completely untouched throughout.
