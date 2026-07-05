# Painter

The Painter provides high-level 2D rendering primitives, specifically designed to interoperate seamlessly with the Surface and Dirty Region engines.

## Features

- Abstracted pixel formatting (defaults to ARGB 32-bit).
- Native awareness of clipping boundaries.

## API

- `painter_draw_pixel`
- `painter_draw_line`
- `painter_draw_rect`
- `painter_fill_rect`
- `painter_draw_surface`

The Painter guarantees that it will never write a pixel outside of the defined surface boundary or the explicit clip rectangle passed by the compositor.
