#include "../include/bdr_api.h"
#include "kernel/botree/runtime/include/dre_api.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/core/memory/heap/include/heap.h"

#define MAX_DESKTOP_SESSIONS 4

static BDrSession s_sessions[MAX_DESKTOP_SESSIONS];
static uint32_t   s_next_session_id = 1;

BDrSession* BDR_CreateSession(uint32_t user_id) {
    for (int i = 0; i < MAX_DESKTOP_SESSIONS; i++) {
        if (!s_sessions[i].active) {
            memset(&s_sessions[i], 0, sizeof(BDrSession));
            s_sessions[i].active = true;
            s_sessions[i].session_id = s_next_session_id++;
            s_sessions[i].user_id = user_id;
            s_sessions[i].solid_color = 0xFF004E98; // Windows XP Bliss Blue
            s_sessions[i].wallpaper_mode = BDR_WALLPAPER_STRETCH;

            // Connect to BO-TREE DRE Runtime Engine!
            s_sessions[i].dre_runtime = BDeRuntime_Create(user_id, DRE_VIEW_MODE_ICONS);
            BDeRuntime_Open(s_sessions[i].dre_runtime, "virtual://Desktop");

            // Populate Standard Windows XP Desktop System Icons
            BDR_AddIcon(&s_sessions[i], "This PC", "virtual://ThisPC", BDR_ICON_TYPE_THIS_PC, 0, 0);
            BDR_AddIcon(&s_sessions[i], "Recycle Bin", "virtual://RecycleBin", BDR_ICON_TYPE_RECYCLE, 0, 1);
            BDR_AddIcon(&s_sessions[i], "Documents", "virtual://Documents", BDR_ICON_TYPE_FOLDER, 0, 2);

            return &s_sessions[i];
        }
    }
    return NULL;
}

void BDR_DestroySession(BDrSession* session) {
    if (!session || !session->active) return;
    if (session->dre_runtime) {
        BDeRuntime_Destroy(session->dre_runtime);
        session->dre_runtime = NULL;
    }
    session->active = false;
}

int32_t BDR_LoadSession(BDrSession* session) {
    if (!session || !session->active) return -1;
    return BDR_AutoArrangeIcons(session);
}

int32_t BDR_SaveSession(BDrSession* session) {
    if (!session || !session->active) return -1;
    return 0;
}
