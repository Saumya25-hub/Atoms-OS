#include "platform/include/bos_platform.h"

extern void BOS_WindowRegistry_Init(void);
extern void BOS_AppManager_Init(void);
extern void BOS_GlobalEventQueue_Init(void);

BOS_Result BOS_Platform_Initialize(void) {
    BOS_WindowRegistry_Init();
    BOS_AppManager_Init();
    BOS_GlobalEventQueue_Init();
    return BOS_SUCCESS;
}

void BOS_Platform_Shutdown(void) {
    /* Cleanup platform registry resources */
}

const char* BOS_Platform_GetVersionString(void) {
    return "BOS Platform Foundation v1.0.0-PHASE2";
}
