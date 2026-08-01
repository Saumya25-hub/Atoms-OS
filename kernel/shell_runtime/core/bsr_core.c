#include "../include/bsr_api.h"
#include "kernel/botree/runtime/include/dre_api.h"
#include "kernel/desktop_runtime/include/bdr_api.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/drivers/display/display.h"

static BSR_Runtime s_bsr_pool[BSR_MAX_RUNTIMES];
static uint32_t    s_next_bsr_id = 1;
static bool        s_bsr_inited = false;

int32_t BSR_Init(void) {
    if (s_bsr_inited) return 0;
    memset(s_bsr_pool, 0, sizeof(s_bsr_pool));

    display_print("[BSR] Initializing Master Shell Runtime Engine V1.0...\n");

    // Register Default System File Associations
    BSR_RegisterAssociation(".txt", "Text Viewer", "/apps/text_viewer.elf");
    BSR_RegisterAssociation(".md", "Text Viewer", "/apps/text_viewer.elf");
    BSR_RegisterAssociation(".bmp", "Image Viewer", "/apps/image_viewer.elf");
    BSR_RegisterAssociation(".elf", "Process Spawner", "/kernel/core/loader");

    s_bsr_inited = true;
    return 0;
}

BSR_Runtime* BSR_CreateRuntime(uint32_t owner_pid, BSR_AppType app_type) {
    for (int i = 0; i < BSR_MAX_RUNTIMES; i++) {
        if (!s_bsr_pool[i].active) {
            memset(&s_bsr_pool[i], 0, sizeof(BSR_Runtime));
            s_bsr_pool[i].active = true;
            s_bsr_pool[i].runtime_id = s_next_bsr_id++;
            s_bsr_pool[i].owner_pid = owner_pid;
            s_bsr_pool[i].app_type = app_type;

            // Bind DRE Directory Runtime
            s_bsr_pool[i].directory = BDeRuntime_Create(owner_pid, DRE_VIEW_MODE_ICONS);

            // Bind Desktop Session if App is Desktop
            if (app_type == BSR_APP_DESKTOP) {
                s_bsr_pool[i].desktop = BDR_CreateSession(owner_pid);
            }

            strcpy(s_bsr_pool[i].current_path, "/");
            return &s_bsr_pool[i];
        }
    }
    return NULL;
}

void BSR_DestroyRuntime(BSR_Runtime* rt) {
    if (!rt || !rt->active) return;
    if (rt->directory) {
        BDeRuntime_Destroy(rt->directory);
        rt->directory = NULL;
    }
    if (rt->desktop) {
        BDR_DestroySession(rt->desktop);
        rt->desktop = NULL;
    }
    rt->active = false;
}
