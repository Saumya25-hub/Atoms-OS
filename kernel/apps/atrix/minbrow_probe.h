/*
 * ATOMS OS / ATRIX ? Minimal Real-Web Browser Probe
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 */

#ifndef KERNEL_APPS_ATRIX_MINBROW_PROBE_H
#define KERNEL_APPS_ATRIX_MINBROW_PROBE_H

#include <stdint.h>
#include <stdbool.h>
#include "kernel/wm/bwe/include/bwe.h"

#ifdef __cplusplus
extern "C" {
#endif

bwe_error_t minbrow_probe_launch(uint32_t* out_win_id);
bwe_error_t minbrow_probe_launch_mode(uint32_t* out_win_id, const char* mode);
void minbrow_probe_close(void);

#ifdef __cplusplus
}
#endif

#endif // KERNEL_APPS_ATRIX_MINBROW_PROBE_H
