#ifndef SYSCALLS_GUI_H
#define SYSCALLS_GUI_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define BOS_GUI_EVENT_ABI_VERSION 1U

typedef enum {
    BOS_GUI_EVENT_NONE        = 0,
    BOS_GUI_EVENT_CLICK       = 1,
    BOS_GUI_EVENT_CLOSE       = 2,
    BOS_GUI_EVENT_KEY_DOWN    = 3,
    BOS_GUI_EVENT_KEY_UP      = 4,
    BOS_GUI_EVENT_MOUSE_MOVE  = 5,
    BOS_GUI_EVENT_MOUSE_DOWN  = 6,
    BOS_GUI_EVENT_MOUSE_UP    = 7,
    BOS_GUI_EVENT_FOCUS_GAIN  = 8,
    BOS_GUI_EVENT_FOCUS_LOST  = 9
} BOS_GUIEventType;

typedef struct {
    uint32_t        abi_version; /* Must match BOS_GUI_EVENT_ABI_VERSION */
    uint32_t        type;        /* BOS_GUIEventType */
    uint32_t        window_id;   /* Target Window ID */
    int32_t         mouse_x;     /* Window-local Mouse X */
    int32_t         mouse_y;     /* Window-local Mouse Y */
    uint32_t        mouse_btn;   /* 1=Left, 2=Right, 4=Middle */
    uint32_t        key_code;    /* Hardware Keycode */
    uint32_t        ascii_char;  /* Printable ASCII char */
    uint32_t        modifiers;   /* Shift=1, Ctrl=2, Alt=4 */
    uint32_t        reserved;    /* 64-bit alignment padding */
} BOS_GUIEvent;

/*
 * =====================================================================
 * ATOMS OS FROZEN GUI SYSCALL ABI V1.0 (Syscall Range: 16 - 23)
 * =====================================================================
 */

uint32_t sys_gui_create_window(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t flags, const char* title);
int      sys_gui_destroy_window(uint32_t win_id);
int      sys_gui_show_window(uint32_t win_id, bool visible);
int      sys_gui_set_bounds(uint32_t win_id, int32_t x, int32_t y, int32_t w, int32_t h);
int      sys_gui_map_surface(uint32_t win_id, uint32_t** out_surface_pixels, uint32_t* out_stride_bytes);
int      sys_gui_invalidate(uint32_t win_id, int32_t x, int32_t y, int32_t w, int32_t h);
int      sys_gui_poll_event(uint32_t win_id, BOS_GUIEvent* out_event);
int      sys_gui_get_screen_info(uint32_t* out_w, uint32_t* out_h, uint32_t* out_bpp);
int      sys_gui_draw_wallpaper(uint32_t win_id, int32_t x, int32_t y, int32_t w, int32_t h);

#endif /* SYSCALLS_GUI_H */
