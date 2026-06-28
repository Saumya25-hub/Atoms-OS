#ifndef SYSCALLS_GUI_H
#define SYSCALLS_GUI_H

#include "bos_gui.h"

uint32_t sys_gui_create_window(int32_t x, int32_t y, int32_t w, int32_t h, const char* title);
uint32_t sys_gui_create_button(uint32_t parent_id, int32_t x, int32_t y, int32_t w, int32_t h, const char* text, void (*cb)(void));
uint32_t sys_gui_create_label(uint32_t parent_id, int32_t x, int32_t y, const char* text, uint32_t color);
uint32_t sys_gui_create_panel(uint32_t parent_id, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
void sys_gui_show_window(uint32_t id);
void sys_gui_set_bounds(uint32_t id, int32_t x, int32_t y, int32_t w, int32_t h);
void sys_gui_set_text(uint32_t id, const char* text);
int sys_gui_get_event(BOS_GUIEvent* event);

#endif
