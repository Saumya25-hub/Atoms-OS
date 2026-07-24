#ifndef ABE_WINDOW_H
#define ABE_WINDOW_H

#include "../../../sdk/include/abe/abe.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ABE_MAX_WINDOWS_REGISTRY 16
#define ABE_MAX_TABS_PER_WINDOW  32

typedef struct {
    ABE_WindowHandle handle;
    uint32_t bosurface_id;
    char title[ABE_MAX_TITLE_LEN];
    int32_t x;
    int32_t y;
    uint32_t width;
    uint32_t height;
    int32_t restore_x;
    int32_t restore_y;
    uint32_t restore_w;
    uint32_t restore_h;
    bool is_fullscreen;
    bool is_active;
    bool is_visible;

    // Window Layout Regions
    struct {
        int32_t x; int32_t y; uint32_t w; uint32_t h;
    } tab_strip_bounds;
    struct {
        int32_t x; int32_t y; uint32_t w; uint32_t h;
    } address_bar_bounds;
    struct {
        int32_t x; int32_t y; uint32_t w; uint32_t h;
    } content_surface_bounds;

    ABE_TabHandle tab_handles[ABE_MAX_TABS_PER_WINDOW];
    uint32_t tab_count;
    ABE_TabHandle active_tab_handle;
} ABE_BrowserWindow;

typedef struct {
    ABE_BrowserWindow windows[ABE_MAX_WINDOWS_REGISTRY];
    uint32_t active_window_count;
    ABE_WindowHandle focused_window;
} ABE_WindowManager;

ABE_Error ABE_Window_Init(void);
ABE_Error ABE_Window_Shutdown(void);

ABE_Error ABE_Window_Create(const char* title, int32_t x, int32_t y, uint32_t width, uint32_t height, ABE_WindowHandle* out_handle);
ABE_Error ABE_Window_Destroy(ABE_WindowHandle handle);
ABE_Error ABE_Window_Resize(ABE_WindowHandle handle, uint32_t width, uint32_t height);
ABE_Error ABE_Window_SetFullscreen(ABE_WindowHandle handle, bool fullscreen);
ABE_BrowserWindow* ABE_Window_Get(ABE_WindowHandle handle);

ABE_Error ABE_Window_AttachTab(ABE_WindowHandle window, ABE_TabHandle tab);
ABE_Error ABE_Window_DetachTab(ABE_WindowHandle window, ABE_TabHandle tab);
ABE_Error ABE_Window_SelectTab(ABE_WindowHandle window, ABE_TabHandle tab);
ABE_Error ABE_Window_RedrawFrame(ABE_WindowHandle handle);

#ifdef __cplusplus
}
#endif

#endif // ABE_WINDOW_H
