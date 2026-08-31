#ifndef START_MENU_H
#define START_MENU_H

#include <stdint.h>
#include <stdbool.h>

extern uint32_t g_start_menu_win_id;
extern bool g_start_menu_open;

void StartMenu_Initialize(void);
void StartMenu_RefreshCache(void);
void StartMenu_Toggle(void);
void StartMenu_Open(void);
void StartMenu_Close(void);

#endif // START_MENU_H
