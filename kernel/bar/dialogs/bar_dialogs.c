#include "../include/bar_api.h"

BARDialogID BAR_CreateDialog(BARWindowID parent_id, const char* title, int32_t w, int32_t h) {
    if (!title) return 0;
    (void)parent_id; (void)w; (void)h;
    return (BARDialogID)0xD1A00001;
}

int32_t BAR_ShowDialog(BARDialogID dlg_id) {
    if (dlg_id == 0) return -1;
    return 0;
}

int32_t BAR_CloseDialog(BARDialogID dlg_id) {
    if (dlg_id == 0) return -1;
    return 0;
}
