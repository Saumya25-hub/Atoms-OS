#ifndef ABE_WEB_SCHEDULER_H
#define ABE_WEB_SCHEDULER_H

#include "../../../sdk/include/abe/abe.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*ABE_FrameCallback)(void* user_data);

ABE_Error ABE_WebScheduler_Init(void);
ABE_Error ABE_WebScheduler_Shutdown(void);

ABE_Error ABE_WebScheduler_RequestAnimationFrame(ABE_JSContextHandle ctx, ABE_FrameCallback cb, void* user_data, uint32_t* out_id);
ABE_Error ABE_WebScheduler_CancelAnimationFrame(uint32_t id);

ABE_Error ABE_WebScheduler_RequestIdleCallback(ABE_JSContextHandle ctx, ABE_FrameCallback cb, void* user_data, uint32_t* out_id);
ABE_Error ABE_WebScheduler_CancelIdleCallback(uint32_t id);

#ifdef __cplusplus
}
#endif

#endif // ABE_WEB_SCHEDULER_H
