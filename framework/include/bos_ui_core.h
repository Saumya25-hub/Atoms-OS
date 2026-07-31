#ifndef BOS_UI_CORE_H
#define BOS_UI_CORE_H

#include "platform/include/bos_types.h"
#include "platform/include/bos_events.h"

typedef struct BOS_UIElement BOS_UIElement;
typedef struct BOS_DrawContext BOS_DrawContext;

typedef enum {
    BOS_ALIGN_LEFT = 0,
    BOS_ALIGN_CENTER,
    BOS_ALIGN_RIGHT,
    BOS_ALIGN_STRETCH
} BOS_HorizontalAlignment;

typedef enum {
    BOS_VALIGN_TOP = 0,
    BOS_VALIGN_CENTER,
    BOS_VALIGN_BOTTOM,
    BOS_VALIGN_STRETCH
} BOS_VerticalAlignment;

typedef struct {
    int32_t left;
    int32_t top;
    int32_t right;
    int32_t bottom;
} BOS_Thickness;

/* UIElement Base Control Class Structure */
struct BOS_UIElement {
    uint32_t                id;
    const char*             type_name;
    BOS_Rect                bounds;            /* Computed screen bounds */
    BOS_Size                desired_size;      /* Measure pass requested size */
    BOS_Thickness           margin;
    BOS_Thickness           padding;
    BOS_HorizontalAlignment halign;
    BOS_VerticalAlignment   valign;
    BOS_Size                min_size;
    BOS_Size                max_size;
    
    bool                    visible;
    bool                    enabled;
    bool                    is_focused;
    bool                    is_hovered;
    bool                    is_pressed;
    bool                    is_layout_valid;

    /* Styling Properties */
    uint32_t                bg_color;
    uint32_t                fg_color;
    uint32_t                border_color;
    uint32_t                border_thickness;
    uint32_t                border_radius;
    float                   opacity;

    /* Hierarchy Tree */
    BOS_UIElement*          parent;
    BOS_UIElement**         children;
    uint32_t                child_count;
    uint32_t                child_capacity;

    /* Virtual Function Table */
    void (*measure)(BOS_UIElement* self, BOS_Size available_size);
    void (*arrange)(BOS_UIElement* self, BOS_Rect final_rect);
    void (*render)(BOS_UIElement* self, BOS_DrawContext* draw_ctx);
    void (*on_event)(BOS_UIElement* self, const BOS_Event* event);
};

void BOS_UIElement_Init(BOS_UIElement* elem, const char* type_name);
void BOS_UIElement_AddChild(BOS_UIElement* parent, BOS_UIElement* child);
void BOS_UIElement_RemoveChild(BOS_UIElement* parent, BOS_UIElement* child);
void BOS_UIElement_InvalidateLayout(BOS_UIElement* elem);

#endif /* BOS_UI_CORE_H */
