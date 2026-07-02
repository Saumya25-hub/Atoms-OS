# Window Lifecycle

## 1. Window Creation
Applications call `BOS_CreateWindow()`.
- `BWE_AllocateWindowSlot()` finds an empty index in the static 1024-array `g_windows`.
- Default flags (`BWE_WINDOW_RESIZABLE | BWE_WINDOW_MOVABLE`) are set.
- A backing `BOCompositorSurface` is registered via `BOCompositor_RegisterSurface()`.
- Initial state is `BWE_STATE_CREATED`.

## 2. Window Registration & Hierarchy
Windows can spawn child controls (`BOS_CreateButton`, `BOS_CreatePanel`).
- The child stores its `parent_id`.
- The parent appends the child ID to its `children[]` array.
- Bounds are calculated relative to the parent (`local_bounds` vs `screen_bounds`).

## 3. Show
`BOS_Show()` changes state to `BWE_STATE_SHOWN`.
- `BOCompositor_Update` marks the surface visible.
- Damage is added to the screen rectangle.

## 4. Move / Resize
- When dragging the titlebar, `BOS_SetBounds()` is called constantly.
- The old bounds are marked dirty, and the new bounds are marked dirty.
- The window is constrained by `min_size` and `max_size`.

## 5. Focus
Clicking a window invokes `BOS_SetFocus()`.
- The previous focused window loses the `BOCOMPOSITOR_FLAG_FOCUSED` flag.
- The new window gains it.
- Titlebar colors swap (Active vs Inactive theme).
- Z-Order is updated via `BOCompositorStack_BringToFront()`.

## 6. Minimize / Maximize
- **Minimize**: Changes state to `BOCOMPOSITOR_STATE_MINIMIZED`. Surface is excluded from the render stack.
- **Maximize**: Current bounds are saved to `restore_bounds`. Window bounds are set to fill the desktop area minus the Taskbar.

## 7. Destroy
`BOS_DestroySurface()`:
- Recursively calls `BOS_DestroySurface()` on all children in `children[]`.
- Calls `BOCompositor_RemoveSurface()`.
- Flags slot as `BWE_STATE_DESTROYED`.
- Bumps the `generation` counter for the slot to prevent ID recycling bugs.
- **Memory Cleanup**: Since everything uses a static pool, there is zero dynamic memory `free()` logic. It purely sets `active = false`.
