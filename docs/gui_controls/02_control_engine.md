# Base Control Engine

The `kernel/gui/controls/control.h` defines the polymorphic `BOSControl` structure. Because ATOMS OS is written in C, we simulate Object-Oriented inheritance by making `BOSControl` the very first struct member of every specific control type.

## Architecture

```c
typedef struct BOSControl {
    int x;
    int y;
    int width;
    int height;
    
    bool visible;
    bool enabled;
    bool focused;
    bool hovered;
    bool pressed;
    
    uint32_t id;
    
    struct BOSControl* parent;
    struct BOSControl* first_child;
    struct BOSControl* next_sibling;
    
    void (*paint)(struct BOSControl* self, struct BOSSurface* surface, const BVRect* clip);
    void (*handle_event)(struct BOSControl* self, const GUIEvent* event);
    void (*destroy)(struct BOSControl* self);
} BOSControl;
```

## Invalidation & Double Buffering
When a control changes state (e.g., a button is hovered), it does not redraw the entire screen. Instead, it calls `control_invalidate(this)`.

This function:
1. Calculates the absolute screen coordinates of the control by traversing up to the window parent.
2. Intersects with the window's clipping bounds.
3. Submits a dirty rectangle to the `Compositor`.
4. The Window Manager's paint loop will eventually redraw just that region by calling `window_redraw()`, which cascades down the control tree invoking `paint()` with the dirty `clip` rect.

## Memory Management
A parent control (like a Panel or Window) owns its children. When a parent is destroyed, `control_destroy_recursive()` is called, which cascades destruction to all children, preventing memory leaks.
