# Rendering Architecture

The Desktop Shell strictly obeys the Window Manager V2 compositing rules.

## No Direct Framebuffer Access

None of the shell components write directly to VBE.

Instead, when an interaction changes a visual state (like hovering over the Start button):
1. The component updates its state enum.
2. It calls `compositor_invalidate_rect(&bounding_box)`.
3. The Compositor wakes up, notes the dirty region.
4. The Compositor asks the Surface Tree (which includes the Desktop, Icons, Taskbar, and Windows) to redraw themselves ONLY if they intersect the dirty region.
5. The Painter clips the drawing commands, ensuring absolute minimal memory bandwidth usage.
