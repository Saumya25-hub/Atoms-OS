# Surface Engine

The Surface Engine is the core structural element of the Window Manager. Everything visible on screen—from the desktop to tooltips—is represented as a `BOSSurface`.

## Struct Definition

```c
struct BOSSurface {
    uint32_t id;
    int x;
    int y;
    int width;
    int height;
    uint32_t* framebuffer;
    bool visible;
    bool dirty;
    uint32_t z_order;
    struct BOSSurface* parent;
    struct BOSSurface* next;
    struct BOSSurface* children;
};
```

## Hierarchy

Surfaces are arranged in a tree structure. The Desktop is the root node. Windows are children of the desktop. Popups are children of windows. This allows for:
- Automatic relative positioning (future).
- Recursive destruction (preventing memory leaks).
- Z-order based traversal during composition.
