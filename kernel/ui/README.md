# UI Module

## Purpose
Manages the minimalist graphical user interface of ATOMS OS Foundation v2. Strictly implements static, square designs with no transparency, blur, or animations. Prioritizes rendering speed and predictable behavior.

## Public API
- `start_menu_init()`
- `start_menu_open()`
- `start_menu_close()`
- `start_menu_draw()`
- `start_menu_handle_mouse()`

## Dependencies
- Drawing primitives from the internal Compositor / Graphics Driver (`draw_rect`, `draw_text`).
- `horse_engine.h` (Specifically `horse_launch()`) for delegating application launching without coupling the UI to the Kernel.

## TODO
- Implement the static Task Panel.
- Connect hitboxes with real compositor hover states (solid color highlighting).
- Tie `draw_rect` / `draw_text` stubs to actual BOSurface framebuffer writing.
