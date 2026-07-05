# Focus Model

The focus model is enforced via strict Z-ordering in `interaction_engine.c`.

1. **Start Menu (Top Priority)**: If visible, it intercepts mouse clicks first.
2. **Taskbar**: Always available to capture Start Button toggles.
3. **Application Windows**: Captured by `window_get_at_point()`. The topmost window is brought to the front of the drawing tree and receives focus.
4. **Desktop Icons & Background**: The absolute bottom layer.
