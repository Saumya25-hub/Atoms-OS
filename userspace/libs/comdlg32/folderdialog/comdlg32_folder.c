#include "../include/comdlg32_api.h"
#include "kernel/core/lib/include/string.h"

LPVOID SHBrowseForFolder(BROWSEINFO* lpbi) {
    if (!lpbi) return NULL;
    if (lpbi->pszDisplayName) {
        strcpy(lpbi->pszDisplayName, "C:\\Documents");
    }
    return (LPVOID)1;
}
