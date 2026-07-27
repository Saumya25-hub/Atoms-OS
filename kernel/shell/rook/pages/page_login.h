#ifndef PAGE_LOGIN_H
#define PAGE_LOGIN_H

#include "kernel/shell/rook/include/rook.h"

/*
 * ♜ ATOMS OS Login Page (Page 3: ROOK_PAGE_LOGIN)
 * Single Login Page Architecture with two internal states:
 * State 1: Lock Screen
 * State 2: Sign In
 */

rook_page_t* rook_page_login_get(void);

#endif /* PAGE_LOGIN_H */
