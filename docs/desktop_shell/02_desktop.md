# Desktop Component

The Desktop is the root of the Surface Engine tree. 

It is initialized via `desktop_init()` which creates a surface covering the entire screen dimension.

## Responsibilities

- Filling the background with `COLOR_DESKTOP_BG` (teal).
- Serving as the parent anchor for all Application Windows, the Taskbar, and Desktop Icons.
- Acting as the ultimate fallback for hit testing (clearing selections when empty space is clicked).
