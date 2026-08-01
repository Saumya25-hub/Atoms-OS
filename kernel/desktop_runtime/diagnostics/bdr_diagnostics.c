#include "../include/bdr_api.h"
#include "kernel/core/lib/include/string.h"

void BDR_GetDiagnostics(BDrSession* session, BDrDiagnostics* out_diag) {
    if (!out_diag) return;
    memset(out_diag, 0, sizeof(BDrDiagnostics));

    if (session && session->active) {
        out_diag->active_icons = session->icon_count;
        out_diag->selected_icons = 0;
        for (uint32_t i = 0; i < session->icon_count; i++) {
            if (session->icons[i].selected) out_diag->selected_icons++;
        }
        out_diag->active_notifications = session->notification_count;
        out_diag->grid_utilization_pct = (session->icon_count * 100) / BDR_MAX_ICONS;
        out_diag->memory_bytes = sizeof(BDrSession);
    }
}
