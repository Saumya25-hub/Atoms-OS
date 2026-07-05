#ifndef GUI_CONTROLS_CONTROL_H
#define GUI_CONTROLS_CONTROL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "kernel/gui/surface/surface.h"
#include "kernel/gui/events/gui_event.h"
#include "bovisual/Include/bovisual_types.h"

// Forward declaration
struct BOSWindow;
struct BOSControl;

// Function pointer types for polymorphism
typedef void (*ControlPaintFunc)(struct BOSControl* self, struct BOSSurface* surface, const BVRect* clip);
typedef void (*ControlEventFunc)(struct BOSControl* self, const GUIEvent* event);
typedef void (*ControlDestroyFunc)(struct BOSControl* self);

// Base Control Structure
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
    
    struct BOSWindow* window; // The window this control belongs to
    
    struct BOSControl* parent;
    struct BOSControl* first_child;
    struct BOSControl* next_sibling;
    
    // Polymorphic methods
    ControlPaintFunc paint;
    ControlEventFunc handle_event;
    ControlDestroyFunc destroy;
    
    // Custom data pointer for derived controls to store extra state if needed
    void* priv_data;
} BOSControl;

// Base Control API
void control_init(BOSControl* control);
void control_set_position(BOSControl* control, int x, int y);
void control_set_size(BOSControl* control, int width, int height);
void control_set_visible(BOSControl* control, bool visible);
void control_set_enabled(BOSControl* control, bool enabled);

// Tree manipulation
void control_add_child(BOSControl* parent, BOSControl* child);
void control_remove_child(BOSControl* parent, BOSControl* child);

// Invalidation & Rendering
void control_invalidate(BOSControl* control);
void control_paint_children(BOSControl* parent, struct BOSSurface* surface, const BVRect* clip);

// Memory Management
void control_destroy_recursive(BOSControl* control);

// Utility: get absolute position on screen
void control_get_absolute_position(BOSControl* control, int* abs_x, int* abs_y);
// Utility: hit test relative to control
BOSControl* control_hit_test(BOSControl* root, int x, int y);

#endif // GUI_CONTROLS_CONTROL_H
