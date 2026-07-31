#include "platform/include/bos_runtime.h"

BOS_Result BOS_RunApplication(BOS_App* app) {
    if (!app) return BOS_ERROR_INVALID_ARGUMENT;

    app->is_running = true;

    /* Startup Hook */
    if (app->config.on_startup) {
        BOS_Result res = app->config.on_startup(app);
        if (res != BOS_SUCCESS) {
            app->is_running = false;
            return res;
        }
    }

    /* Main Deterministic Event Dispatch Loop */
    BOS_Event event;
    while (app->is_running) {
        if (BOS_EventQueue_Pop(&app->event_queue, &event) == BOS_SUCCESS) {
            if (app->config.on_event) {
                app->config.on_event(app, &event);
            }
        } else {
            /* Yield CPU time slice to scheduler when queue is idle */
            /* sys_yield() placeholder for system integration */
        }
    }

    /* Shutdown Hook */
    if (app->config.on_shutdown) {
        app->config.on_shutdown(app);
    }

    return BOS_SUCCESS;
}
