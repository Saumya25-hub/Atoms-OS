#include "../include/brt_api.h"
#include "kernel/shell_runtime/include/bsr_api.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/drivers/display/display.h"

static BRTRuntime s_brt_pool[BRT_MAX_RUNTIMES];
static uint32_t   s_next_brt_id = 1;
static bool       s_brt_inited = false;

int32_t BRT_Init(void) {
    if (s_brt_inited) return 0;
    memset(s_brt_pool, 0, sizeof(s_brt_pool));

    display_print("[BRT] Initializing BO-TREE Runtime Integration Layer V1.0...\n");

    // Initialize BSR Master Controller
    BSR_Init();

    s_brt_inited = true;
    return 0;
}

BRTRuntime* BRT_CreateRuntime(uint32_t owner_pid, BRT_AppType app_type) {
    for (int i = 0; i < BRT_MAX_RUNTIMES; i++) {
        if (!s_brt_pool[i].active) {
            memset(&s_brt_pool[i], 0, sizeof(BRTRuntime));
            s_brt_pool[i].active = true;
            s_brt_pool[i].runtime_id = s_next_brt_id++;
            s_brt_pool[i].owner_pid = owner_pid;
            s_brt_pool[i].app_type = app_type;
            s_brt_pool[i].ref_count = 1;

            // Map app_type enum to BSR_AppType
            BSR_AppType bsr_type = BSR_APP_EXPLORER;
            if (app_type == BRT_APP_DESKTOP) bsr_type = BSR_APP_DESKTOP;
            else if (app_type == BRT_APP_TERMINAL) bsr_type = BSR_APP_TERMINAL;
            else if (app_type == BRT_APP_DIALOG) bsr_type = BSR_APP_DIALOG;
            else if (app_type == BRT_APP_BROWSER) bsr_type = BSR_APP_BROWSER;

            // Bind to BSR Master Controller
            s_brt_pool[i].shell_rt = BSR_CreateRuntime(owner_pid, bsr_type);

            strcpy(s_brt_pool[i].current_path, "/");
            return &s_brt_pool[i];
        }
    }
    return NULL;
}

void BRT_DestroyRuntime(BRTRuntime* rt) {
    if (!rt || !rt->active) return;
    if (rt->shell_rt) {
        BSR_DestroyRuntime(rt->shell_rt);
        rt->shell_rt = NULL;
    }
    rt->active = false;
}
