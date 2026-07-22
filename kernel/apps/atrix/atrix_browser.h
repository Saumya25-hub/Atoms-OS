#ifndef KERNEL_APPS_ATRIX_BROWSER_H
#define KERNEL_APPS_ATRIX_BROWSER_H

#include <stdint.h>
#include <stdbool.h>
#include "kernel/wm/bwe/include/bwe.h"

// ATRIX Browser Lifecycle API
bwe_error_t atrix_browser_launch(uint32_t* out_win_id);
void atrix_browser_close(void);

#endif // KERNEL_APPS_ATRIX_BROWSER_H
