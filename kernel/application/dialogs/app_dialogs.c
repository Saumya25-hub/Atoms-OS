#include "app_dialogs.h"
#include "kernel/application/window_api/app_window_api.h"
#include "kernel/wm/bwe/include/bwe.h"

bwe_error_t ATOMS_ShowMessageBox(uint32_t owner_app_id, const char* title, const char* message, ATOMS_DialogIcon icon, uint32_t* out_dialog_win) {
    (void)icon;
    uint32_t dlg_win = 0;
    bwe_error_t err = ATOMS_CreateWindowForApp(owner_app_id, 300, 200, 400, 180, title ? title : "Message", &dlg_win);
    if (err != BWE_SUCCESS) return err;

    uint32_t lbl = 0;
    BOS_CreateLabel(dlg_win, 20, 40, message ? message : "", 0xFFFFFFFF, &lbl);

    uint32_t btn_ok = 0;
    BOS_CreateButton(dlg_win, 150, 120, 100, 30, "OK", 0, &btn_ok);

    if (out_dialog_win) *out_dialog_win = dlg_win;
    return BWE_SUCCESS;
}

bwe_error_t ATOMS_ShowFileOpenDialog(uint32_t owner_app_id, const char* title, const char* filter, uint32_t* out_dialog_win) {
    (void)filter;
    uint32_t dlg_win = 0;
    bwe_error_t err = ATOMS_CreateWindowForApp(owner_app_id, 250, 150, 500, 350, title ? title : "Open File", &dlg_win);
    if (err != BWE_SUCCESS) return err;

    uint32_t lbl = 0;
    BOS_CreateLabel(dlg_win, 20, 40, "Select a file from filesystem:", 0xFFCBD5E1, &lbl);

    if (out_dialog_win) *out_dialog_win = dlg_win;
    return BWE_SUCCESS;
}
