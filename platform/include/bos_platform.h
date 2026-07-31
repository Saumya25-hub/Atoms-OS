#ifndef BOS_PLATFORM_H
#define BOS_PLATFORM_H

/* Main umbrella header for BOS Application Platform applications */

#include "bos_types.h"
#include "bos_events.h"
#include "bos_window.h"
#include "bos_runtime.h"
#include "bos_app_manager.h"

BOS_Result  BOS_Platform_Initialize(void);
void        BOS_Platform_Shutdown(void);
const char* BOS_Platform_GetVersionString(void);
void        BOS_Platform_Log(const char* module, const char* fmt, ...);

#endif /* BOS_PLATFORM_H */
