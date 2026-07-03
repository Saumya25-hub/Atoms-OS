# Desktop Shell Simplification

The Desktop Shell (desktop_shell.c) is now extremely minimalist.
Its sole responsibilities are:
1. Initialize the Desktop Background/Wallpaper.
2. Render hardcoded Desktop Icons.
3. Pass double-clicks to Horse Engine.

No application logic or window tracking happens here anymore.
