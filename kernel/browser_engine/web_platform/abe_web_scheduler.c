#include "abe_web_scheduler.h"
#include "../diagnostics/abe_diagnostics.h"

static uint32_t g_next_frame_id = 1000;
static bool g_web_scheduler_initialized = false;

ABE_Error ABE_WebScheduler_Init(void) {
    g_web_scheduler_initialized = true;
    ABE_Log(ABE_LOG_INFO, "WEBSCHEDULER", "ABE Browser Scheduler Engine initialized");
    return ABE_SUCCESS;
}

ABE_Error ABE_WebScheduler_Shutdown(void) {
    g_web_scheduler_initialized = false;
    return ABE_SUCCESS;
}

ABE_Error ABE_WebScheduler_RequestAnimationFrame(ABE_JSContextHandle ctx, ABE_FrameCallback cb, void* user_data, uint32_t* out_id) {
    if (!g_web_scheduler_initialized || !cb || !out_id) return ABE_ERR_INVALID_PARAM;
    *out_id = g_next_frame_id++;
    ABE_Diag_RecordAnimationFrame();
    return ABE_SUCCESS;
}

ABE_Error ABE_WebScheduler_CancelAnimationFrame(uint32_t id) {
    if (!g_web_scheduler_initialized) return ABE_ERR_NOT_INITIALIZED;
    return ABE_SUCCESS;
}

ABE_Error ABE_WebScheduler_RequestIdleCallback(ABE_JSContextHandle ctx, ABE_FrameCallback cb, void* user_data, uint32_t* out_id) {
    if (!g_web_scheduler_initialized || !cb || !out_id) return ABE_ERR_INVALID_PARAM;
    *out_id = g_next_frame_id++;
    return ABE_SUCCESS;
}

ABE_Error ABE_WebScheduler_CancelIdleCallback(uint32_t id) {
    if (!g_web_scheduler_initialized) return ABE_ERR_NOT_INITIALIZED;
    return ABE_SUCCESS;
}
