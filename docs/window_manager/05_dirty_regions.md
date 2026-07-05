# Dirty Regions Engine

Full-frame repaints limit composition speed to how fast the CPU/DMA can copy memory. For 60 FPS performance, Window Manager V2 relies on partial invalidation.

## Concept

When a surface changes (moves, resizes, animates), it submits a "Dirty Rectangle" to the `DirtyRegion` engine.

## API

- `InvalidateRect(rect)`: Adds a dirty rectangle to the current frame's tracking list.
- `MergeDirtyRects()`: Optimizes multiple overlapping invalidation zones into a bounding box or disjoint regions to minimize draw calls.
- `ClipRect(dest, src, bounds)`: Generates the intersection of what needs to be drawn and what is actually invalid on screen.

Instead of redrawing everything, the Compositor uses the aggregated `DirtyRegion` to only update pixels that have changed.
