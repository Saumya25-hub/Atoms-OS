#ifndef ABE_JS_PROMISE_H
#define ABE_JS_PROMISE_H

#include "../../../sdk/include/abe/abe.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PROMISE_STATE_PENDING = 0,
    PROMISE_STATE_FULFILLED,
    PROMISE_STATE_REJECTED
} ABE_JSPromiseState;

typedef struct {
    uint32_t handle;
    ABE_JSPromiseState state;
    ABE_JSValue result;
    bool in_use;
} ABE_JSPromise;

typedef void (*ABE_MicrotaskCallback)(void* data);

typedef struct {
    ABE_MicrotaskCallback callback;
    void* data;
} ABE_JSMicrotask;

ABE_Error ABE_JSPromise_Init(void);
ABE_Error ABE_JSPromise_Shutdown(void);

ABE_Error ABE_JSPromise_Create(uint32_t* out_promise_handle);
ABE_Error ABE_JSPromise_Resolve(uint32_t promise_handle, const ABE_JSValue* val);
ABE_Error ABE_JSPromise_Reject(uint32_t promise_handle, const ABE_JSValue* err_val);

ABE_Error ABE_JS_QueueMicrotask(ABE_MicrotaskCallback cb, void* data);
uint32_t  ABE_JS_ProcessMicrotasks(void);

#ifdef __cplusplus
}
#endif

#endif // ABE_JS_PROMISE_H
