#ifndef BOS_H
#define BOS_H

/* Official Umbrella C SDK Header for BOS OS */

#include "bos_version.h"
#include "bos_res.h"
#include "bos_pack.h"
#include "platform/include/bos_platform.h"
#include "framework/include/bos_ui.h"

/* High-level C SDK Helper Wrappers */
BOS_Result  BOS_SDK_Init(const char* app_name, const char* version);
int         BOS_SDK_Run(void);
void        BOS_SDK_Quit(int exit_code);
BOS_Window* BOS_SDK_CreateWindow(int32_t x, int32_t y, uint32_t w, uint32_t h, const char* title);
void        BOS_SDK_ShowWindow(BOS_Window* win);

#endif /* BOS_H */
