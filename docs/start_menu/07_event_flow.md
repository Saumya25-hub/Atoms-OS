# Event Flow

1. User clicks the Start Button.
2. Taskbar captures click, fires `GUI_EVENT_STARTMENU_OPEN` (or calls toggle directly).
3. Start Menu state changes to `VISIBLE`.
4. Compositor invalidates the Start Menu rect.
5. User clicks "Calculator".
6. Start Menu captures click, calls `app_launch()`, and hides itself.
7. Calculator app calls `window_create()`.
8. Window is added to the tree. Compositor invalidates the new window's area.
9. Painter draws the Calculator.
