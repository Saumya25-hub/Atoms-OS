#ifndef ATOMS_APP_DIALOGS_H
#define ATOMS_APP_DIALOGS_H

#include <stdint.h>
#include <stdbool.h>
#include "kernel/wm/bwe/include/bwe.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// ATOMS Application Dialogs Framework
// ============================================================

typedef enum {
    ATOMS_DIALOG_ICON_INFO,
    ATOMS_DIALOG_ICON_WARNING,
    ATOMS_DIALOG_ICON_ERROR,
    ATOMS_DIALOG_ICON_QUESTION
} ATOMS_DialogIcon;

typedef enum {
    ATOMS_DIALOG_RESULT_OK,
    ATOMS_DIALOG_RESULT_CANCEL,
    ATOMS_DIALOG_RESULT_YES,
    ATOMS_DIALOG_RESULT_NO
} ATOMS_DialogResult;

bwe_error_t ATOMS_ShowMessageBox(uint32_t owner_app_id, const char* title, const char* message, ATOMS_DialogIcon icon, uint32_t* out_dialog_win);
bwe_error_t ATOMS_ShowFileOpenDialog(uint32_t owner_app_id, const char* title, const char* filter, uint32_t* out_dialog_win);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_APP_DIALOGS_H
