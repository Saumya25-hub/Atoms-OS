#include "../include/comdlg32_api.h"

BOOL PrintDlg(PRINTDLG* lppd) {
    if (!lppd) return false;
    lppd->nCopies = 1;
    return true;
}
