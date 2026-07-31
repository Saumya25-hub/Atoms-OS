#ifndef BOSFSR_CORE_H
#define BOSFSR_CORE_H

#include <stdint.h>
#include <stdbool.h>
#include "kernel/wm/bwe/include/bwe.h"

typedef enum {
    BOSFSR_DOCK_NONE,
    BOSFSR_DOCK_TOP,
    BOSFSR_DOCK_BOTTOM,
    BOSFSR_DOCK_LEFT,
    BOSFSR_DOCK_RIGHT,
    BOSFSR_DOCK_FILL
} BOSFSR_Dock;

typedef enum {
    BOSFSR_STATE_NORMAL,
    BOSFSR_STATE_HOVERED,
    BOSFSR_STATE_PRESSED,
    BOSFSR_STATE_FOCUSED,
    BOSFSR_STATE_DISABLED,
    BOSFSR_STATE_SELECTED
} BOSFSR_ControlState;

typedef struct BOSFSR_Control BOSFSR_Control;

struct BOSFSR_Control {
    char name[64];
    int32_t x, y, width, height;
    int32_t corner_radius;
    int32_t border_width;
    uint32_t back_color;
    uint32_t fore_color;
    uint32_t border_color;
    uint32_t start_color;
    uint32_t end_color;
    bool is_gradient;
    bool has_shadow;
    bool visible;
    bool enabled;
    BOSFSR_Dock dock;
    BOSFSR_ControlState state;
    
    BOSFSR_Control* parent;
    BOSFSR_Control** children;
    uint32_t child_count;
    uint32_t child_capacity;

    void (*paint)(BOSFSR_Control* self, const BVFramebuffer* fb, int parent_x, int parent_y);
    void (*layout)(BOSFSR_Control* self);
};

BOSFSR_Control* bosfsr_create_control(const char* name, int x, int y, int w, int h);
void bosfsr_add_child(BOSFSR_Control* parent, BOSFSR_Control* child);
void bosfsr_destroy_control(BOSFSR_Control* ctrl);
void bosfsr_paint_tree(BOSFSR_Control* root, const BVFramebuffer* fb, int parent_x, int parent_y);

#endif /* BOSFSR_CORE_H */
