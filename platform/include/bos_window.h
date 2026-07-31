#ifndef BOS_WINDOW_H
#define BOS_WINDOW_H

#include "bos_types.h"

/* Window Creation Flags */
#define BOS_WINDOW_FLAG_NORMAL      0x00000000U
#define BOS_WINDOW_FLAG_RESIZABLE   0x00000001U
#define BOS_WINDOW_FLAG_NO_BORDER   0x00000002U
#define BOS_WINDOW_FLAG_ALWAYS_TOP  0x00000004U
#define BOS_WINDOW_FLAG_MODAL       0x00000008U

/* Window Configuration Parameter Struct */
typedef struct {
    int32_t     x;
    int32_t     y;
    uint32_t    width;
    uint32_t    height;
    const char* title;
    uint32_t    flags;
} BOS_WindowConfig;

/* Window Management Lifecycle Functions */
BOS_Result BOS_CreateWindow(const BOS_WindowConfig* config, BOS_WindowHandle* out_handle);
BOS_Result BOS_DestroyWindow(BOS_WindowHandle handle);
BOS_Result BOS_CloseWindow(BOS_WindowHandle handle);

/* Visibility & State Functions */
BOS_Result BOS_ShowWindow(BOS_WindowHandle handle);
BOS_Result BOS_HideWindow(BOS_WindowHandle handle);
BOS_Result BOS_SetFocus(BOS_WindowHandle handle);
BOS_Result BOS_GetFocus(BOS_WindowHandle* out_handle);

/* Geometry & Position Functions */
BOS_Result BOS_MoveWindow(BOS_WindowHandle handle, int32_t x, int32_t y);
BOS_Result BOS_ResizeWindow(BOS_WindowHandle handle, uint32_t width, uint32_t height);
BOS_Result BOS_GetWindowBounds(BOS_WindowHandle handle, BOS_Rect* out_bounds);

#endif /* BOS_WINDOW_H */
