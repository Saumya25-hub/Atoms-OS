#include "platform/include/bos_runtime.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/core/memory/heap/include/heap.h"

static BOS_App* g_current_app_instance = NULL;

BOS_Result BOS_InitApplication(const BOS_AppConfig* config, BOS_App** out_app) {
    if (!config || !out_app) return BOS_ERROR_INVALID_ARGUMENT;
    if (!config->app_name) return BOS_ERROR_INVALID_ARGUMENT;

    BOS_App* app = (BOS_App*)kmalloc(sizeof(BOS_App));
    if (!app) return BOS_ERROR_OUT_OF_MEMORY;

    memset(app, 0, sizeof(BOS_App));
    app->handle = 0x00010001U;
    app->config = *config;
    app->is_running = false;
    app->exit_code = 0;
    app->pid = 1;

    BOS_EventQueue_Init(&app->event_queue);
    g_current_app_instance = app;

    *out_app = app;
    return BOS_SUCCESS;
}

BOS_App* BOS_GetCurrentApplication(void) {
    return g_current_app_instance;
}

void BOS_QuitApplication(BOS_App* app, int exit_code) {
    if (!app) app = g_current_app_instance;
    if (app) {
        app->is_running = false;
        app->exit_code = exit_code;
    }
}
