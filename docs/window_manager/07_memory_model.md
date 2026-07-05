# Memory Model

Window Manager V2 employs a strict ownership-based memory model to guarantee zero leaks.

## Rules of Ownership

1. **Window Owns Surface**: Creating a window allocates a surface. Destroying a window frees that surface.
2. **Surface Owns Framebuffer**: A surface's framebuffer is dynamically allocated based on its dimensions. Resizing frees the old buffer and creates a new one. Destruction frees the buffer.
3. **Desktop Owns Window Tree**: The desktop surface is the root of the tree.
4. **Recursive Teardown**: Calling `surface_destroy()` on a node automatically calls `surface_destroy()` on all of its children, guaranteeing that orphaned UI elements are flushed from memory.
