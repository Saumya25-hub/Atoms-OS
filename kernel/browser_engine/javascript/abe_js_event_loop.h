#ifndef ABE_JS_EVENT_LOOP_H
#define ABE_JS_EVENT_LOOP_H

#include "../../../sdk/include/abe/abe.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t ABE_JSTimerID;
typedef void (*ABE_TimerCallback)(void* user_data);

typedef struct {
    ABE_JSTimerID id;
    ABE_TimerCallback cb;
    void* user_data;
    uint32_t delay_ms;
    bool is_interval;
    bool in_use;
} ABE_JSTimer;

ABE_Error ABE_JSEventLoop_Init(void);
ABE_Error ABE_JSEventLoop_Shutdown(void);

ABE_Error ABE_JSEventLoop_RunTick(ABE_JSContextHandle ctx_handle);

ABE_JSTimerID ABE_SetTimeout(ABE_TimerCallback cb, uint32_t delay_ms, void* user_data);
ABE_Error     ABE_ClearTimeout(ABE_JSTimerID timer_id);
ABE_JSTimerID ABE_SetInterval(ABE_TimerCallback cb, uint32_t delay_ms, void* user_data);
ABE_Error     ABE_ClearInterval(ABE_JSTimerID timer_id);

#ifdef __cplusplus
}
#endif

#endif // ABE_JS_EVENT_LOOP_H
