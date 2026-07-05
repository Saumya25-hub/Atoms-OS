# Rendering Architecture

The Start Menu utilizes the same Compositing pipeline as standard windows.

By leveraging `surface_add_child()` to the desktop root, and explicitly manipulating the linked list to ensure it's at the front, we trick the Compositor into painting the Start Menu on top of everything else during the back-to-front rendering pass.

When the Start Menu is closed, `compositor_invalidate_surface()` is called. The Start Menu surface is skipped in the next render pass (because `visible = false`), and the applications/desktop underneath are redrawn perfectly inside that bounding box.
