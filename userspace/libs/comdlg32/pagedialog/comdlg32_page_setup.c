#include "../include/comdlg32_api.h"

BOOL PageSetupDlg(PAGESETUPDLG* lppsd) {
    if (!lppsd) return false;
    lppsd->ptPaperSize.x = 210; // A4 Width
    lppsd->ptPaperSize.y = 297; // A4 Height
    return true;
}
