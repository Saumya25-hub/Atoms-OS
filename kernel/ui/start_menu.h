#ifndef START_MENU_H
#define START_MENU_H

#include <stdint.h>
#include <stdbool.h>

/* Initializes the Start Menu state */
void start_menu_init(void);

/* Opens the Start Menu (if not already open) */
void start_menu_open(void);

/* Closes the Start Menu */
void start_menu_close(void);

/* Renders the Start Menu to the screen buffer */
void start_menu_draw(void);

/* Processes mouse interaction with the Start Menu */
void start_menu_handle_mouse(int x, int y, bool clicked);

#endif // START_MENU_H
