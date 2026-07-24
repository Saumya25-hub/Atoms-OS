#ifndef ABE_WEB_NAVIGATOR_H
#define ABE_WEB_NAVIGATOR_H

#include "../../../sdk/include/abe/abe.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char user_agent[256];
    char platform[64];
    char language[32];
    uint32_t hardware_concurrency;
    bool cookie_enabled;
    bool on_line;
} ABE_WebNavigatorInfo;

ABE_Error ABE_WebNavigator_Init(void);
ABE_Error ABE_WebNavigator_Shutdown(void);

ABE_Error ABE_WebNavigator_GetInfo(ABE_WebNavigatorInfo* out_info);

#ifdef __cplusplus
}
#endif

#endif // ABE_WEB_NAVIGATOR_H
