# Compositor

The Compositor is responsible for translating the Surface Tree into a final flattened image on the hardware framebuffer.

## Responsibilities
- Z-order sorting (bottom-to-top traversal).
- Dirty rectangle clipping (via the Dirty Region Engine).
- Composition from the backbuffer to the frontbuffer (`vbe_swap_page`).

## Pipeline
1. Global invalidation queue (Dirty Regions) is evaluated.
2. For each dirty rectangle, the Compositor traverses the Surface Tree from the Desktop Root up to the Cursor.
3. It instructs the Painter to draw each surface, bounded by the intersection of the dirty rectangle and the surface's rectangle.
4. Once all dirty rectangles are rendered to the back buffer, the front buffer and back buffer are swapped.

## Integration
The Compositor interfaces directly with the hardware via VBE APIs but relies entirely on the Painter for pixel-level operations.
