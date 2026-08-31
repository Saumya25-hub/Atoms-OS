#ifndef START_MENU_H
#define START_MENU_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
    int32_t search_x;
    int32_t search_y;
    int32_t search_w;
    int32_t search_h;
    int32_t nav_x;
    int32_t nav_y;
    int32_t nav_w;
    int32_t grid_x;
    int32_t grid_y;
    int32_t grid_w;
    int32_t footer_y;
} StartMenu_Layout;

extern uint32_t g_start_menu_win_id;
extern bool g_start_menu_open;

void StartMenu_Initialize(void);
void StartMenu_RefreshCache(void);
void StartMenu_Toggle(void);
void StartMenu_Open(void);
void StartMenu_Close(void);
const StartMenu_Layout* StartMenu_GetLayout(void);

#endif // START_MENU_H
