# Hit Testing Mechanics

## How It Works
`BWE_HitTest(window_id, screen_x, screen_y)` evaluates a single coordinate against a window's spatial properties.

1. **Bounds Check:** Fast-fail if coordinates fall outside `screen_bounds`.
2. **Borderless Exemption:** Returns `BWE_HIT_CLIENT` immediately for borderless windows.
3. **8-Way Corners:** Checks a 12x12 pixel region on all four corners (`BWE_HIT_CORNER_TL`, etc.).
4. **8-Way Borders:** Checks a 5px band along edges (`BWE_HIT_BORDER_T`, etc.).
5. **Titlebar & Buttons:** Slices the top 5px to 35px range. Evaluates Close, Maximize, and Minimize buttons using hardcoded right-aligned pixel offsets.
6. **Client Area:** Everything remaining falls through to `BWE_HIT_CLIENT`.

## Z-Order Impact
In `BWE_PumpEvents`, Hit Testing is performed dynamically:
```c
for (int32_t i = (int32_t)g_z_stack_count - 1; i >= 0; i--) {
    uint32_t win_id = g_z_order_stack[i];
    BWE_HitZone hit = BWE_HitTest(win_id, bwe_ev.data.mouse.x, bwe_ev.data.mouse.y);
```
It linearly scans the entire Z-order stack from top to bottom. Once a top-level window matches, it recurses through **all** of its children to find the leaf node. This means Hit Testing evaluates layout bounds frame-by-frame instead of caching a quad-tree or collision map.
