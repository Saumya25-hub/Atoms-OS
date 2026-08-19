#include "../include/syscalls_gui.h"

#define SYS_GUI_CREATE_WINDOW       16U
#define SYS_GUI_DESTROY_WINDOW      17U
#define SYS_GUI_SHOW_WINDOW         18U
#define SYS_GUI_SET_BOUNDS          19U
#define SYS_GUI_MAP_SURFACE         20U
#define SYS_GUI_INVALIDATE          21U
#define SYS_GUI_POLL_EVENT          22U
#define SYS_GUI_GET_SCREEN_INFO     23U
#define SYS_GUI_DRAW_WALLPAPER      24U

uint32_t sys_gui_create_window(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t flags, const char* title) {
    uint32_t id = 0;
    register uint64_t r10 asm("r10") = (uint64_t)h;
    register uint64_t r8  asm("r8")  = (uint64_t)flags;
    register uint64_t r9  asm("r9")  = (uint64_t)title;
    __asm__ volatile("syscall"
        : "=a"(id)
        : "a"(SYS_GUI_CREATE_WINDOW), "D"(x), "S"(y), "d"(w), "r"(r10), "r"(r8), "r"(r9)
        : "rcx", "r11", "memory");
    return id;
}

int sys_gui_destroy_window(uint32_t win_id) {
    int res = 0;
    __asm__ volatile("syscall"
        : "=a"(res)
        : "a"(SYS_GUI_DESTROY_WINDOW), "D"(win_id)
        : "rcx", "r11", "memory");
    return res;
}

int sys_gui_show_window(uint32_t win_id, bool visible) {
    int res = 0;
    __asm__ volatile("syscall"
        : "=a"(res)
        : "a"(SYS_GUI_SHOW_WINDOW), "D"(win_id), "S"((uint32_t)visible)
        : "rcx", "r11", "memory");
    return res;
}

int sys_gui_set_bounds(uint32_t win_id, int32_t x, int32_t y, int32_t w, int32_t h) {
    int res = 0;
    register uint64_t r10 asm("r10") = (uint64_t)w;
    register uint64_t r8  asm("r8")  = (uint64_t)h;
    __asm__ volatile("syscall"
        : "=a"(res)
        : "a"(SYS_GUI_SET_BOUNDS), "D"(win_id), "S"(x), "d"(y), "r"(r10), "r"(r8)
        : "rcx", "r11", "memory");
    return res;
}

int sys_gui_map_surface(uint32_t win_id, uint32_t** out_surface_pixels, uint32_t* out_stride_bytes) {
    int res = 0;
    __asm__ volatile("syscall"
        : "=a"(res)
        : "a"(SYS_GUI_MAP_SURFACE), "D"(win_id), "S"(out_surface_pixels), "d"(out_stride_bytes)
        : "rcx", "r11", "memory");
    return res;
}

int sys_gui_invalidate(uint32_t win_id, int32_t x, int32_t y, int32_t w, int32_t h) {
    int res = 0;
    register uint64_t r10 asm("r10") = (uint64_t)w;
    register uint64_t r8  asm("r8")  = (uint64_t)h;
    __asm__ volatile("syscall"
        : "=a"(res)
        : "a"(SYS_GUI_INVALIDATE), "D"(win_id), "S"(x), "d"(y), "r"(r10), "r"(r8)
        : "rcx", "r11", "memory");
    return res;
}

int sys_gui_poll_event(uint32_t win_id, BOS_GUIEvent* out_event) {
    if (!out_event) return 0;
    out_event->abi_version = BOS_GUI_EVENT_ABI_VERSION;
    int res = 0;
    __asm__ volatile("syscall"
        : "=a"(res)
        : "a"(SYS_GUI_POLL_EVENT), "D"(win_id), "S"(out_event), "d"((uint32_t)sizeof(BOS_GUIEvent))
        : "rcx", "r11", "memory");
    return res;
}

int sys_gui_get_screen_info(uint32_t* out_w, uint32_t* out_h, uint32_t* out_bpp) {
    int res = 0;
    __asm__ volatile("syscall"
        : "=a"(res)
        : "a"(SYS_GUI_GET_SCREEN_INFO), "D"(out_w), "S"(out_h), "d"(out_bpp)
        : "rcx", "r11", "memory");
    return res;
}

int sys_gui_draw_wallpaper(uint32_t win_id, int32_t x, int32_t y, int32_t w, int32_t h) {
    int res = 0;
    register int32_t r10 asm("r10") = w;
    register int32_t r8  asm("r8")  = h;
    __asm__ volatile("syscall"
        : "=a"(res)
        : "a"(SYS_GUI_DRAW_WALLPAPER), "D"(win_id), "S"(x), "d"(y), "r"(r10), "r"(r8)
        : "rcx", "r11", "memory");
    return res;
}
