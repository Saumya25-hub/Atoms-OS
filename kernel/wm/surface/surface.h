#ifndef BOSURFACE_SURFACE_H
#define BOSURFACE_SURFACE_H

#include <stdint.h>
#include <stdbool.h>
#include "bovisual/Include/events.h"

// ============================================================
// BWE Error Codes (from BWE_API_SPECIFICATION.md)
// ============================================================
#define BWE_SUCCESS   0x0000
#define BWE0001       0x1001  // Invalid Surface Pointer / ID
#define BWE0002       0x1002  // Invalid Parent ID
#define BWE0003       0x1003  // Render Failed
#define BWE0004       0x1004  // Memory Allocation Failed
#define BWE0005       0x1005  // Focus Error
#define BWE0006       0x1006  // Dirty Region Overflow
#define BWE0007       0x1007  // Invalid Control ID
#define BWE0008       0x1008  // Surface Already Exists

// ============================================================
// BWE Surface Flags
// ============================================================
#define BWE_FLAG_VISIBLE        (1 << 0)
#define BWE_FLAG_FOCUSED        (1 << 1)
#define BWE_FLAG_ALPHA          (1 << 2)
#define BWE_FLAG_DOUBLEBUFFERED (1 << 3)
#define BWE_FLAG_DRAGGABLE      (1 << 4)
#define BWE_FLAG_MAXIMIZED      (1 << 5)

// ============================================================
// BWE Surface States (Lifecycle)
// ============================================================
typedef enum {
    BWE_STATE_CREATED,
    BWE_STATE_VISIBLE,
    BWE_STATE_HIDDEN,
    BWE_STATE_FOCUSED,
    BWE_STATE_INACTIVE,
    BWE_STATE_MINIMIZED,
    BWE_STATE_MAXIMIZED,
    BWE_STATE_DESTROYED
} BWE_SurfaceState;

// ============================================================
// BWE Surface Geometry (Absolute Screen Coordinates)
// ============================================================
typedef struct {
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
} BWE_Rect;

// ============================================================
// BWE Layout & Composition
// ============================================================
void BWE_ComputeScreenBounds(void);
void BWE_Compose(void);

// ============================================================
// BOFRAMES & BOHEART ENGINE AUTHORITY
// ============================================================
#include "bovisual/Include/bovisual_types.h"

// Raw Input Capture & Pulse Authority
void BOHeart_InputCapture(const BVEvent* ev);
void BOHeart_Pulse(const BVFramebuffer* hw_fb);

void BOF_BeginAtomicFrame(void);
void BOF_ComposeFullFrame(void);
void BOF_ComposeDirtyOnly(void); // Backwards compatibility alias to BOF_ComposeFullFrame
void BOF_EndAtomicFrame(const BVFramebuffer* hw_fb);

typedef struct {
    int32_t mouse_x;
    int32_t mouse_y;
    uint8_t mouse_buttons;
    uint32_t focused_surface;
    uint32_t hovered_surface;
    uint32_t active_drag_surface;
} FrameSnapshot;

extern FrameSnapshot g_frame_snapshot;
void BOF_CaptureSnapshot(int32_t mouse_x, int32_t mouse_y, uint8_t buttons);
FrameSnapshot CaptureFullSystemState(void);
void BOF_BeginFrameLock(void);
void BOF_EndFrameLock(void);

// ============================================================
// BWE Surface Structure
// ============================================================
#define BWE_MAX_SURFACES  64
#define BWE_MAX_CHILDREN  16
#define BWE_DESKTOP_ID    0

typedef enum {
    BWE_TYPE_SURFACE,
    BWE_TYPE_PANEL,
    BWE_TYPE_BUTTON,
    BWE_TYPE_LABEL,
    BWE_TYPE_TEXTBOX,
    BWE_TYPE_WALLPAPER,
    BWE_TYPE_TASKBAR,
    BWE_TYPE_DESKTOP_ICON
} BWE_SurfaceType;

typedef struct {
    uint32_t bg_color;
} BWE_PanelData;

typedef struct {
    char text[128];
    uint32_t text_color;
    uint32_t bg_color;
    bool is_pressed;
    bool is_hovered;
    void (*on_click)(uint32_t button_id);
    uint64_t user_callback;
} BWE_ButtonData;

typedef struct {
    char text[128];
    uint32_t text_color;
    bool transparent;
} BWE_LabelData;

typedef struct {
    char text[128];
    char placeholder[128];
    uint32_t bg_color;
    uint32_t text_color;
} BWE_TextboxData;

typedef struct BWE_Surface {
    // Identity
    uint32_t            id;
    bool                active;        // Is this slot in use?
    BWE_SurfaceType     type;

    // Tree: Parent-Child Relationships
    uint32_t            parent_id;
    uint32_t            children[BWE_MAX_CHILDREN];
    uint32_t            child_count;
    uint32_t            z_order;       // Higher = drawn later (on top)

    // Geometry (relative to parent)
    BWE_Rect            local_bounds;
    // Geometry (absolute screen coordinates, computed by compositor)
    BWE_Rect            screen_bounds;
    // Geometry (saved bounds for window restore)
    BWE_Rect            restore_bounds;

    // State
    BWE_SurfaceState    state;
    uint32_t            flags;
    bool                is_dirty;
    BWE_Rect            old_screen_bounds;

    // Visual Styling Properties (Rounded Corners & Gradients)
    uint32_t            corner_radius;      // 0 = rectangular (default), > 0 = rounded corner radius in px
    uint8_t             gradient_mode;      // 0 = NONE (solid), 1 = VERTICAL, 2 = HORIZONTAL
    uint32_t            gradient_color_end; // Secondary color for gradient fills

    // Owner (for future process tracking)
    uint32_t            owner_pid;

    // Control metadata
    union {
        BWE_PanelData   panel;
        BWE_ButtonData  button;
        BWE_LabelData   label;
        BWE_TextboxData textbox;
    } control_data;

    // Hooks
    void (*on_event)(uint32_t surface_id, const BVEvent* event);
    void (*on_render)(struct BWE_Surface* surface);
} BWE_Surface;

typedef uint32_t bwe_error_t;

// ============================================================
// BWE Public API (from BWE_API_SPECIFICATION.md)
// ============================================================

// Core Lifecycle
void        BOSurface_Init(void);
bwe_error_t BOS_CreateSurface(uint32_t parent_id, uint32_t x, uint32_t y,
                               uint32_t width, uint32_t height,
                               uint32_t flags, uint32_t* out_surface_id);
bwe_error_t BOS_DestroySurface(uint32_t surface_id);

// Window Manager APIs (Phase 7)
bwe_error_t BOS_CreateWindow(int32_t x, int32_t y, int32_t width, int32_t height, const char* title, uint32_t* out_id);
bwe_error_t BOS_MinimizeSurface(uint32_t surface_id);
bwe_error_t BOS_MaximizeSurface(uint32_t surface_id);
bwe_error_t BOS_RestoreSurface(uint32_t surface_id);
bwe_error_t BOS_CloseSurface(uint32_t surface_id);

// Desktop Shell APIs (Phase 8)
bwe_error_t BOS_SetWallpaper(uint32_t color);
bwe_error_t BOS_CreateTaskbar(void);
bwe_error_t BOS_CreateDesktopIcon(uint32_t x, uint32_t y, const char* label, void (*on_click)(uint32_t), uint32_t* out_id);

// Control Generation APIs
bwe_error_t BOS_CreatePanel(uint32_t parent_id, uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color_bg, uint32_t* out_control_id);
bwe_error_t BOS_CreateButton(uint32_t parent_id, uint32_t x, uint32_t y, uint32_t width, uint32_t height, const char* text, void (*on_click)(uint32_t), uint32_t* out_control_id);
bwe_error_t BOS_CreateLabel(uint32_t parent_id, uint32_t x, uint32_t y, const char* text, uint32_t color_fg, uint32_t* out_control_id);
bwe_error_t BOS_CreateTextbox(uint32_t parent_id, uint32_t x, uint32_t y, uint32_t width, uint32_t height, const char* placeholder, uint32_t* out_control_id);
bwe_error_t BOS_SetText(uint32_t target_id, const char* text);

// Visibility
bwe_error_t BOS_Show(uint32_t target_id);
bwe_error_t BOS_Hide(uint32_t target_id);
void        BOS_InvalidateSurface(uint32_t surface_id);

// Geometry
bwe_error_t BOS_SetBounds(uint32_t target_id, uint32_t x, uint32_t y,
                           uint32_t width, uint32_t height);

// Focus Engine
bwe_error_t BOS_SetFocus(uint32_t surface_id);
uint32_t    BOS_GetFocus(void);
bwe_error_t BOS_ClearFocus(void);
uint32_t    BOS_GetActiveSurface(void);

// Events & Interaction
uint32_t    BWE_HitTest(int32_t screen_x, int32_t screen_y);
void        BOS_ProcessEvent(const BVEvent* event);

// Compositor
void        BWE_ComputeScreenBounds(void);
void        BWE_CollectDamage(void);
void        BWE_Compose(void);

// Accessors
BWE_Surface* BWE_GetSurface(uint32_t surface_id);
uint32_t     BWE_GetSurfaceCount(void);
uint32_t     BWE_GetHoverSurfaceID(void);
bool         BWE_IsDragging(void);
uint32_t     BWE_GetDragSurfaceID(void);

// Phase Tests
void BOS_Test_Phase10_Terminal(void);
void BOS_Test_Phase11_Explorer(void);
void BOS_Test_Phase12_TextViewer(void);
void BOS_Test_Phase13_BOSXLoader(void);
void BOS_Test_Phase1(void);
void BOS_Test_Phase2(void);
void BOS_Test_Phase3(void);
void BOS_Test_Phase4(void);
void BOS_Test_Phase5(void);
void BOS_Test_Phase6(void);

extern uint32_t g_current_creating_pid;
bwe_error_t BOS_CloseSurfacesByPID(uint32_t pid);
uint32_t BOS_CountSurfacesByPID(uint32_t pid);

// BODEBUG Engine (Phase 17.6)
extern bool BOS_DEBUG_MODE;
extern uint32_t bwe_capture_surface_id;
void bodebug_dump(void);

#endif // BOSURFACE_SURFACE_H
