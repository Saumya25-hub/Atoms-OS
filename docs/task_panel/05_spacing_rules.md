# Spacing Rules

## Immutable Margins
- **Left Padding (Start Menu):** 100 pixels.
- **Right Padding (Clock Area):** 70 pixels.
- **Inter-Button Spacing:** 10 pixels gap between every generated task button.
- **Top Padding:** 10 pixels.
- **Button Height:** 30 pixels.
- **Baseline Padding:** 10 pixels below button.

## Equation
`Total Button Footprint = (btn_w * count) + (10 * (count - 1))`

The layout engine ensures this sum never exceeds `panel_width - 170`.
