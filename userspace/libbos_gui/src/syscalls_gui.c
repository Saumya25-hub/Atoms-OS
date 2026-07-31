#include "../include/syscalls_gui.h"

#define SYS_GUI_CREATE_WINDOW   30
#define SYS_GUI_CREATE_BUTTON   31
#define SYS_GUI_CREATE_LABEL    32
#define SYS_GUI_CREATE_TEXTBOX  33
#define SYS_GUI_CREATE_PANEL    34
#define SYS_GUI_SHOW_WINDOW     35
#define SYS_GUI_SET_TEXT        36
#define SYS_GUI_SET_BOUNDS      37
#define SYS_GUI_DESTROY         38
#define SYS_GUI_GET_EVENT       39

uint32_t sys_gui_create_window(int32_t x, int32_t y, int32_t w, int32_t h, const char* title) {
    uint32_t id;
    register uint64_t r10 asm("r10") = (uint64_t)h;
    register uint64_t r8 asm("r8") = (uint64_t)title;
    __asm__ volatile("syscall" : "=a"(id) : "a"(SYS_GUI_CREATE_WINDOW), "D"(x), "S"(y), "d"(w), "r"(r10), "r"(r8) : "rcx", "r11", "memory");
    return id;
}

uint32_t sys_gui_create_button(uint32_t parent_id, int32_t x, int32_t y, int32_t w, int32_t h, const char* text, void (*cb)(void)) {
    uint64_t ext[3];
    ext[0] = h;
    ext[1] = (uint64_t)text;
    ext[2] = (uint64_t)cb;
    uint32_t id;
    register uint64_t r10 asm("r10") = (uint64_t)w;
    register uint64_t r8 asm("r8") = (uint64_t)ext;
    __asm__ volatile("syscall" : "=a"(id) : "a"(SYS_GUI_CREATE_BUTTON), "D"(parent_id), "S"(x), "d"(y), "r"(r10), "r"(r8) : "rcx", "r11", "memory");
    return id;
}

uint32_t sys_gui_create_label(uint32_t parent_id, int32_t x, int32_t y, const char* text, uint32_t color) {
    uint32_t id;
    register uint64_t r10 asm("r10") = (uint64_t)text;
    register uint64_t r8 asm("r8") = (uint64_t)color;
    __asm__ volatile("syscall" : "=a"(id) : "a"(SYS_GUI_CREATE_LABEL), "D"(parent_id), "S"(x), "d"(y), "r"(r10), "r"(r8) : "rcx", "r11", "memory");
    return id;
}

uint32_t sys_gui_create_panel(uint32_t parent_id, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
    uint64_t ext[2];
    ext[0] = h;
    ext[1] = color;
    uint32_t id;
    register uint64_t r10 asm("r10") = (uint64_t)w;
    register uint64_t r8 asm("r8") = (uint64_t)ext;
    __asm__ volatile("syscall" : "=a"(id) : "a"(SYS_GUI_CREATE_PANEL), "D"(parent_id), "S"(x), "d"(y), "r"(r10), "r"(r8) : "rcx", "r11", "memory");
    return id;
}

void sys_gui_show_window(uint32_t id) {
    __asm__ volatile("syscall" : : "a"(SYS_GUI_SHOW_WINDOW), "D"(id) : "rcx", "r11", "memory");
}

int sys_gui_get_event(BOS_GUIEvent* event) {
    uint32_t res;
    __asm__ volatile("syscall" : "=a"(res) : "a"(SYS_GUI_GET_EVENT), "D"(event) : "rcx", "r11", "memory");
    return (int)res;
}

void sys_gui_set_text(uint32_t id, const char* text) {
    uint32_t res;
    __asm__ volatile("syscall" : "=a"(res) : "a"(SYS_GUI_SET_TEXT), "D"(id), "S"(text) : "rcx", "r11", "memory");
}

void sys_gui_set_bounds(uint32_t id, int32_t x, int32_t y, int32_t w, int32_t h) {
    uint32_t res;
    register uint64_t r10 asm("r10") = (uint64_t)w;
    register uint64_t r8 asm("r8") = (uint64_t)h;
    __asm__ volatile("syscall" : "=a"(res) : "a"(SYS_GUI_SET_BOUNDS), "D"(id), "S"(x), "d"(y), "r"(r10), "r"(r8) : "rcx", "r11", "memory");
}

#define SYS_GUI_SET_CORNER_RADIUS 43
#define SYS_GUI_SET_GRADIENT      44

void sys_gui_set_corner_radius(uint32_t control_id, uint32_t radius) {
    uint32_t res;
    __asm__ volatile("syscall" : "=a"(res) : "a"(SYS_GUI_SET_CORNER_RADIUS), "D"(control_id), "S"(radius) : "rcx", "r11", "memory");
}

void sys_gui_set_gradient(uint32_t control_id, uint32_t color_start, uint32_t color_end, uint8_t mode) {
    uint32_t res;
    register uint64_t r10 asm("r10") = (uint64_t)mode;
    __asm__ volatile("syscall" : "=a"(res) : "a"(SYS_GUI_SET_GRADIENT), "D"(control_id), "S"(color_start), "d"(color_end), "r"(r10) : "rcx", "r11", "memory");
}
