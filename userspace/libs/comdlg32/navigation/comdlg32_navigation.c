#include "../include/comdlg32_api.h"

BOOL NavigateDialog(uint32_t dialogHandle, const char* targetPath) {
    (void)dialogHandle; (void)targetPath;
    return true;
}

BOOL RefreshDialog(uint32_t dialogHandle) {
    (void)dialogHandle;
    return true;
}
