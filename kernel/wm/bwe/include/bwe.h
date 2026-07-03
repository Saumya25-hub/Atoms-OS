#ifndef BWE_H
#define BWE_H

#include "bwe_types.h"
#include "bwe_events.h"
#include "bovisual/Include/bovisual_types.h"
#include "bovisual/Include/events.h"

// Configuration Constants
#define BWE_MAX_WINDOWS 1024
#define BWE_WINDOW_SLOT_MASK (BWE_MAX_WINDOWS - 1)
#define BWE_WINDOW_GEN_SHIFT 12
#define BWE_MAX_CHILDREN 128
#define BWE_DESKTOP_ID  0


// Forward declaration of BWE_Window
typedef struct BWE_Window BWE_Window;
typedef BWE_Window BWE_Control; // Unified OOP abstraction

// BWE Window / Control Structure
struct BWE_Window {
    // Identity & Hierarchy
    uint32_t            id;                 // Globally unique Window/Control ID
    uint32_t            parent_id;          // Parent surface ID (0 = Desktop)
    uint32_t            owner_pid;          // Owner process ID (for resource isolation)
    uint32_t            children[BWE_MAX_CHILDREN]; // Dynamic child surface array
    uint32_t            child_count;        // Count of active child surfaces
    uint32_t            sibling_index;      // Index of this child in the parent's children array
    uint32_t            z_order;            // Rendering depth index
    BWE_SurfaceType     type;               // Type (Surface, Window, Button, etc.)
    BWE_SurfaceState    state;              // Lifecycle state enum

    // Geometry System
    BWE_Rect            local_bounds;       // Bounds relative to parent coordinate space
    BWE_Rect            screen_bounds;      // Absolute bounds in screen space
    BWE_Rect            restore_bounds;     // Saved geometry before maximize/fullscreen
    BWE_Rect            min_size;           // Minimum size bounds (prevent shrink to zero)
    BWE_Rect            max_size;           // Maximum size bounds

    // Layout engine properties
    BWE_Padding         margins;            // Margin spacing outside control bounds
    BWE_Padding         padding;            // Padding spacing inside control bounds
    BWE_DockMode        dock_mode;          // Alignment docking layout mode

    // Flags & Opacity
    uint32_t            flags;              // Behavior and styling flags
    uint8_t             opacity;            // Alpha rendering value (0 = transparent, 255 = opaque)
    bool                is_dirty;           // Invalidation state indicator
    BWE_Rect            old_screen_bounds;  // Previous frame screen bounds (for damage clearing)

    // Data Customization
    void*               user_data;          // Custom user-app context data
    
    // Control Union data (used for widgets state)
    union {
        struct {
            uint32_t bg_color;
        } panel;
        struct {
            char     text[128];
            uint32_t text_color;
            uint32_t bg_color;
            bool     is_pressed;
            bool     is_hovered;
            void     (*on_click)(uint32_t btn_id);
        } button;
        struct {
            char     text[128];
            uint32_t text_color;
            bool     transparent;
        } label;
        struct {
            char     text[128];
            char     placeholder[128];
            uint32_t cursor_pos;
            uint32_t bg_color;
            uint32_t text_color;
        } textbox;
        struct {
            char     text[128];
            bool     checked;
            void     (*on_toggle)(uint32_t chk_id, bool is_checked);
        } checkbox;
        struct {
            char     text[128];
            bool     selected;
            uint32_t group_id;
            void     (*on_select)(uint32_t rad_id);
        } radiobutton;
        struct {
            int32_t  min;
            int32_t  max;
            int32_t  value;
        } progressbar;
        struct {
            bool     vertical;
            int32_t  min;
            int32_t  max;
            int32_t  value;
            int32_t  page_size;
            bool     is_thumb_dragging;
            int32_t  drag_start_val;
            int32_t  drag_start_pos;
            void     (*on_scroll)(uint32_t scr_id, int32_t val);
        } scrollbar;
        struct {
            char     items[16][64];
            uint32_t item_count;
            int32_t  selected_index;
            int32_t  scroll_offset;
            void     (*on_select)(uint32_t list_id, int32_t idx);
            void     (*on_double_click)(uint32_t list_id, int32_t idx);
        } listview;
        struct {
            char     nodes[16][64];
            uint32_t node_count;
            bool     expanded[16];
            int32_t  selected_index;
            int32_t  parent_node_index[16];
            void     (*on_node_select)(uint32_t tree_id, int32_t node_idx);
        } treeview;
        struct {
            void     (*on_paint_canvas)(uint32_t canvas_id, const BVFramebuffer* fb, const BWE_Rect* clip);
        } canvas;
    } control_data;

    // Hooks & Event Callbacks
    void (*on_event)(uint32_t window_id, const BWE_Event* event);
    void (*on_render)(struct BWE_Window* self);
};

// Error Codes
typedef uint32_t bwe_error_t;
#define BWE_SUCCESS   0x0000
#define BWE0001       0x1001  // Invalid Window / Surface Pointer or ID
#define BWE0002       0x1002  // Invalid Parent ID
#define BWE0003       0x1003  // Render Failed
#define BWE0004       0x1004  // Memory Pool Allocation Failed
#define BWE0005       0x1005  // Focus Transition Error
#define BWE0006       0x1006  // Invalidation Queue / Region Overflow
#define BWE0007       0x1007  // Invalid Control ID
#define BWE0008       0x1008  // Surface / Window ID Already Exists



// ============================================================
// Core BWE APIs
// ============================================================

// Engine & Lifecycle Management
bwe_error_t BWE_Initialize(void);
bwe_error_t BOS_CreateSurface(uint32_t parent_id, uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t flags, uint32_t* out_id);
bwe_error_t BOS_DestroySurface(uint32_t window_id);
bwe_error_t BOS_CreateWindow(int32_t x, int32_t y, int32_t width, int32_t height, const char* title, uint32_t* out_id);
bwe_error_t BOS_Show(uint32_t window_id);
bwe_error_t BOS_Hide(uint32_t window_id);

// Registry & Hierarchy Manager
BWE_Window* BWE_GetWindow(uint32_t window_id);
bool        BWE_ValidateWindow(uint32_t window_id);
uint32_t    BWE_GetWindowCount(void);
bwe_error_t BWE_EnumerateWindows(uint32_t* out_ids, uint32_t max_count, uint32_t* out_count);
bool        BWE_HasAncestor(uint32_t window_id, uint32_t ancestor_id);

// Focus Manager
bwe_error_t BOS_SetFocus(uint32_t window_id);
uint32_t    BOS_GetFocus(void);
bwe_error_t BOS_ClearFocus(void);
uint32_t    BWE_GetActiveWindow(void);

// Z-Order Manager
bwe_error_t BWE_BringToFront(uint32_t window_id);
bwe_error_t BWE_SendToBack(uint32_t window_id);
bwe_error_t BWE_UpdateZOrders(void);

// Geometry & Dirty Flag Tracker
bwe_error_t BOS_SetBounds(uint32_t window_id, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
bwe_error_t BOS_GetBounds(uint32_t window_id, BWE_Rect* out_bounds);
bwe_error_t BWE_InvalidateWindow(uint32_t window_id);

// ============================================================
// Layout Engine Subsystem
// ============================================================
void        BWE_UpdateLayout(uint32_t parent_id);

// ============================================================
// Theme Engine Subsystem
// ============================================================
typedef enum {
    BWE_THEME_WINDOW_BG,
    BWE_THEME_WINDOW_BORDER_ACTIVE,
    BWE_THEME_WINDOW_BORDER_INACTIVE,
    BWE_THEME_TITLEBAR_ACTIVE,
    BWE_THEME_TITLEBAR_INACTIVE,
    BWE_THEME_TEXT,
    BWE_THEME_CONTROL_BG,
    BWE_THEME_CONTROL_BORDER,
    BWE_THEME_SELECTION_BG,
    BWE_THEME_SELECTION_TEXT,
    BWE_THEME_ACCENT
} BWE_ThemeToken;

uint32_t    BWE_ThemeGetColor(BWE_ThemeToken token);
void        BWE_ThemeSetDark(bool dark);

// ============================================================
// Compositor Subsystem APIs
// ============================================================
void        BWE_ComposeFrame(const BVFramebuffer* hw_fb);
void        BWE_ClipPush(BWE_Rect rect);
void        BWE_ClipPop(void);
bool        BWE_GetClip(BWE_Rect* out_rect);

// ============================================================
// Paint Engine Primitive APIs
// ============================================================
void        BWE_FillRect(const BVFramebuffer* fb, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
void        BWE_DrawRect(const BVFramebuffer* fb, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color, uint32_t thickness);
void        BWE_DrawLine(const BVFramebuffer* fb, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color);
void        BWE_DrawText(const BVFramebuffer* fb, const char* text, int32_t x, int32_t y, uint32_t color, BWE_Font* font);
void        BWE_DrawBitmap(const BVFramebuffer* fb, const uint32_t* pixels, int32_t dest_x, int32_t dest_y, int32_t dest_w, int32_t dest_h, int32_t src_x, int32_t src_y, int32_t src_w, int32_t src_h, int32_t bmp_pitch);
void        BWE_DrawBorder(const BVFramebuffer* fb, const BWE_Rect* bounds, uint32_t color, bool active);
void        BWE_DrawShadow(const BVFramebuffer* fb, const BWE_Rect* bounds);
void        BWE_DrawTitleBar(const BVFramebuffer* fb, const BWE_Rect* bounds, const char* title, bool active);

// ============================================================
// Drag, Resize & Hit Testing Subsystem APIs
// ============================================================
BWE_HitZone BWE_HitTest(uint32_t window_id, int32_t screen_x, int32_t screen_y);
void        BWE_ProcessMouseInteraction(int32_t x, int32_t y, uint8_t buttons);

// ============================================================
// Control Creation APIs
// ============================================================
bwe_error_t BOS_CreatePanel(uint32_t parent_id, uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color_bg, uint32_t* out_id);
bwe_error_t BOS_CreateButton(uint32_t parent_id, uint32_t x, uint32_t y, uint32_t width, uint32_t height, const char* text, void (*on_click)(uint32_t), uint32_t* out_id);
bwe_error_t BOS_CreateLabel(uint32_t parent_id, uint32_t x, uint32_t y, const char* text, uint32_t color_fg, uint32_t* out_id);
bwe_error_t BOS_CreateTextbox(uint32_t parent_id, uint32_t x, uint32_t y, uint32_t width, uint32_t height, const char* placeholder, uint32_t* out_id);
bwe_error_t BOS_CreateCheckbox(uint32_t parent_id, uint32_t x, uint32_t y, uint32_t width, uint32_t height, const char* text, void (*on_toggle)(uint32_t, bool), uint32_t* out_id);
bwe_error_t BOS_CreateRadioButton(uint32_t parent_id, uint32_t x, uint32_t y, uint32_t width, uint32_t height, const char* text, uint32_t group_id, void (*on_select)(uint32_t), uint32_t* out_id);
bwe_error_t BOS_CreateProgressBar(uint32_t parent_id, uint32_t x, uint32_t y, uint32_t width, uint32_t height, int32_t min, int32_t max, uint32_t* out_id);
bwe_error_t BOS_CreateScrollBar(uint32_t parent_id, uint32_t x, uint32_t y, uint32_t width, uint32_t height, bool vertical, int32_t min, int32_t max, void (*on_scroll)(uint32_t, int32_t), uint32_t* out_id);
bwe_error_t BOS_CreateListView(uint32_t parent_id, uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t* out_id);
bwe_error_t BOS_ListView_AddItem(uint32_t list_id, const char* item);
bwe_error_t BOS_CreateTreeView(uint32_t parent_id, uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t* out_id);
bwe_error_t BOS_TreeView_AddNode(uint32_t tree_id, const char* name, int32_t parent_node_idx, int32_t* out_node_idx);
bwe_error_t BOS_CreateCanvas(uint32_t parent_id, uint32_t x, uint32_t y, uint32_t width, uint32_t height, void (*on_paint)(uint32_t, const BVFramebuffer*, const BWE_Rect*), uint32_t* out_id);

// Event Queue Interface
#define BWE_EVENT_QUEUE_SIZE 1024
typedef struct {
    BWE_Event events[BWE_EVENT_QUEUE_SIZE];
    uint32_t  head;
    uint32_t  tail;
    uint32_t  count;
} BWE_EventQueue;

bwe_error_t BWE_EventQueue_Push(const BWE_Event* event);
bwe_error_t BWE_EventQueue_Pop(BWE_Event* out_event);
bwe_error_t BWE_EventQueue_Peek(BWE_Event* out_event);
void        BWE_EventQueue_Clear(void);

// Event Loop Integration
void        BOS_ProcessEvent(const BVEvent* event);

#endif // BWE_H
