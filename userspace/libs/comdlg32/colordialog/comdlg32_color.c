#include "../include/comdlg32_api.h"

BOOL ChooseColor(CHOOSECOLOR* lpcc) {
    if (!lpcc) return false;
    lpcc->rgbResult = 0x0000FF00; // Default green
    return true;
}
