#include "abe_js_promise.h"
#include "../diagnostics/abe_diagnostics.h"
#include "kernel/core/lib/include/string.h"

#define ABE_JS_MAX_PROMISES 128
#define ABE_JS_MAX_MICROTASKS 256

static ABE_JSPromise g_promises[ABE_JS_MAX_PROMISES];
static ABE_JSMicrotask g_microtask_queue[ABE_JS_MAX_MICROTASKS];
static uint32_t g_microtask_head = 0;
static uint32_t g_microtask_tail = 0;
static uint32_t g_next_promise_id = 6000;
static bool g_js_promise_initialized = false;

ABE_Error ABE_JSPromise_Init(void) {
    memset(g_promises, 0, sizeof(g_promises));
    memset(g_microtask_queue, 0, sizeof(g_microtask_queue));
    g_microtask_head = 0;
    g_microtask_tail = 0;
    g_js_promise_initialized = true;
    ABE_Log(ABE_LOG_INFO, "JSPROMISE", "ABE JavaScript Promise Engine & Microtask Queue initialized");
    return ABE_SUCCESS;
}

ABE_Error ABE_JSPromise_Shutdown(void) {
    g_js_promise_initialized = false;
    return ABE_SUCCESS;
}

ABE_Error ABE_JSPromise_Create(uint32_t* out_promise_handle) {
    if (!g_js_promise_initialized || !out_promise_handle) return ABE_ERR_INVALID_PARAM;

    for (uint32_t i = 0; i < ABE_JS_MAX_PROMISES; i++) {
        if (!g_promises[i].in_use) {
            ABE_JSPromise* p = &g_promises[i];
            memset(p, 0, sizeof(ABE_JSPromise));
            p->handle = (g_next_promise_id++) | (i << 16);
            p->state = PROMISE_STATE_PENDING;
            p->in_use = true;

            *out_promise_handle = p->handle;
            return ABE_SUCCESS;
        }
    }
    return ABE_ERR_RESOURCE_EXHAUSTED;
}

ABE_Error ABE_JSPromise_Resolve(uint32_t promise_handle, const ABE_JSValue* val) {
    if (!g_js_promise_initialized || promise_handle == ABE_INVALID_HANDLE) return ABE_ERR_INVALID_PARAM;
    uint32_t slot = (promise_handle >> 16) & 0xFFFF;
    if (slot >= ABE_JS_MAX_PROMISES) return ABE_ERR_INVALID_PARAM;

    ABE_JSPromise* p = &g_promises[slot];
    if (p->handle == promise_handle && p->in_use && p->state == PROMISE_STATE_PENDING) {
        p->state = PROMISE_STATE_FULFILLED;
        if (val) p->result = *val;
        ABE_Diag_RecordJSPromiseResolved();
        return ABE_SUCCESS;
    }
    return ABE_ERR_INVALID_PARAM;
}

ABE_Error ABE_JSPromise_Reject(uint32_t promise_handle, const ABE_JSValue* err_val) {
    if (!g_js_promise_initialized || promise_handle == ABE_INVALID_HANDLE) return ABE_ERR_INVALID_PARAM;
    uint32_t slot = (promise_handle >> 16) & 0xFFFF;
    if (slot >= ABE_JS_MAX_PROMISES) return ABE_ERR_INVALID_PARAM;

    ABE_JSPromise* p = &g_promises[slot];
    if (p->handle == promise_handle && p->in_use && p->state == PROMISE_STATE_PENDING) {
        p->state = PROMISE_STATE_REJECTED;
        if (err_val) p->result = *err_val;
        return ABE_SUCCESS;
    }
    return ABE_ERR_INVALID_PARAM;
}

ABE_Error ABE_JS_QueueMicrotask(ABE_MicrotaskCallback cb, void* data) {
    if (!g_js_promise_initialized || !cb) return ABE_ERR_INVALID_PARAM;

    uint32_t next_tail = (g_microtask_tail + 1) % ABE_JS_MAX_MICROTASKS;
    if (next_tail == g_microtask_head) {
        return ABE_ERR_RESOURCE_EXHAUSTED; // Queue full
    }

    g_microtask_queue[g_microtask_tail].callback = cb;
    g_microtask_queue[g_microtask_tail].data = data;
    g_microtask_tail = next_tail;
    return ABE_SUCCESS;
}

uint32_t ABE_JS_ProcessMicrotasks(void) {
    if (!g_js_promise_initialized) return 0;
    uint32_t processed = 0;

    while (g_microtask_head != g_microtask_tail) {
        ABE_JSMicrotask task = g_microtask_queue[g_microtask_head];
        g_microtask_head = (g_microtask_head + 1) % ABE_JS_MAX_MICROTASKS;

        if (task.callback) {
            task.callback(task.data);
            processed++;
        }
    }
    return processed;
}
