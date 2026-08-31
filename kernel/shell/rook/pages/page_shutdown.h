#ifndef PAGE_SHUTDOWN_H
#define PAGE_SHUTDOWN_H

#include <stdint.h>
#include <stdbool.h>
#include "kernel/shell/rook/include/rook.h"

/*
 * ♜ ATOMS OS Shutdown & Restart Experience Page
 * Permanent Page ID: ROOK_PAGE_SHUTDOWN (0x0008)
 */

struct rook_page* rook_page_shutdown_get(void);
void rook_page_shutdown_set_mode(bool is_restart);
bool rook_page_shutdown_is_restart(void);

#endif /* PAGE_SHUTDOWN_H */
