#ifndef BWE_TYPES_H
#define BWE_TYPES_H

#include <stdint.h>
#include <stdbool.h>

// BWE Rect (Absolute Screen Coordinates or Local Coordinates)
typedef struct {
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
} BWE_Rect;

// BWE Padding (used for margins/padding)
typedef struct {
    int32_t left;
    int32_t top;
    int32_t right;
    int32_t bottom;
} BWE_Padding;

// BWE Surface / Window / Control Types
typedef enum {
    BWE_TYPE_DESKTOP,
    BWE_TYPE_WINDOW,
    BWE_TYPE_PANEL,
    BWE_TYPE_BUTTON,
    BWE_TYPE_LABEL,
    BWE_TYPE_TEXTBOX,
    BWE_TYPE_WALLPAPER,
    BWE_TYPE_TASKBAR,
    BWE_TYPE_DESKTOP_ICON,
    BWE_TYPE_CHECKBOX,
    BWE_TYPE_RADIOBUTTON,
    BWE_TYPE_PROGRESSBAR,
    BWE_TYPE_SCROLLBAR,
    BWE_TYPE_LISTVIEW,
    BWE_TYPE_TREEVIEW,
    BWE_TYPE_CANVAS
} BWE_SurfaceType;

// BWE Surface / Window Lifecycle States
typedef enum {
    BWE_STATE_CREATED,
    BWE_STATE_INITIALIZED,
    BWE_STATE_SHOWN,
    BWE_STATE_HIDDEN,
    BWE_STATE_ACTIVE,
    BWE_STATE_DEACTIVATED,
    BWE_STATE_DESTROYED
} BWE_SurfaceState;

// BWE Window Object Flags
#define BWE_WINDOW_RESIZABLE    0x00000001
#define BWE_WINDOW_MOVABLE      0x00000002
#define BWE_WINDOW_MODAL        0x00000004
#define BWE_WINDOW_TOPMOST      0x00000008
#define BWE_WINDOW_CHILD        0x00000010
#define BWE_WINDOW_BORDERLESS   0x00000020
#define BWE_WINDOW_TRANSPARENT  0x00000040
#define BWE_WINDOW_FULLSCREEN   0x00000080

// BWE Layout/Docking Modes
typedef enum {
    BWE_DOCK_NONE = 0,
    BWE_DOCK_TOP,
    BWE_DOCK_BOTTOM,
    BWE_DOCK_LEFT,
    BWE_DOCK_RIGHT,
    BWE_DOCK_FILL,
    BWE_DOCK_CENTER
} BWE_DockMode;

// BWE Hit Test Zones
typedef enum {
    BWE_HIT_NONE,
    BWE_HIT_CLIENT,
    BWE_HIT_TITLEBAR,
    BWE_HIT_CLOSE,
    BWE_HIT_MAX,
    BWE_HIT_MIN,
    BWE_HIT_BORDER_T,
    BWE_HIT_BORDER_B,
    BWE_HIT_BORDER_L,
    BWE_HIT_BORDER_R,
    BWE_HIT_CORNER_TL,
    BWE_HIT_CORNER_TR,
    BWE_HIT_CORNER_BL,
    BWE_HIT_CORNER_BR
} BWE_HitZone;

// Dummy structures for forward compatibility with Paint/Theme modules
typedef struct BWE_Font BWE_Font;
typedef struct BWE_Theme BWE_Theme;

#endif // BWE_TYPES_H
