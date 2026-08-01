#include "../include/comdlg32_api.h"

BOOL ChooseFont(CHOOSEFONT* lpcf) {
    if (!lpcf) return false;
    lpcf->iPointSize = 120; // 12 pt
    return true;
}
