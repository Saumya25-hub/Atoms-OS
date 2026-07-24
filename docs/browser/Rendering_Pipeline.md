# ATRIX Browser Engine v1.0 — Rendering Pipeline

## 5-Stage Rendering Pipeline

1. **HTML Parsing -> DOM Tree Creation:** Constructs elemental tree from HTML string stream.
2. **CSS Parsing -> Style Attachment:** Computes CSS properties for matching DOM elements.
3. **Render Tree Building:** Filters visual DOM nodes into layout objects.
4. **Layout & Box Model Computation:** Determines exact (x, y, width, height) bounds via `css_layout.c`.
5. **Compositing & Painting:** Rasterizes visual objects directly onto BWE surface buffers.
