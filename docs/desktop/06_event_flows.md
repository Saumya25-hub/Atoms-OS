# Event Flows

## Task Panel Sync
1. User clicks 'Close' on a Window.
2. Window Manager processes BOS_DestroySurface.
3. WM internally cleans up the surface.
4. WM calls TaskPanel_Update().
5. Task Panel invalidates its BWE Window ID.
6. Compositor repaints the Task Panel in the next frame.
