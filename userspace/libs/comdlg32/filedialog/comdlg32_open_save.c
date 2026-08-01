#include "../include/comdlg32_api.h"
#include "kernel/core/lib/include/string.h"

BOOL GetOpenFileName(OPENFILENAME* lpofn) {
    if (!lpofn || !lpofn->lpstrFile || lpofn->nMaxFile == 0) return false;
    if (strlen(lpofn->lpstrFile) == 0) {
        strcpy(lpofn->lpstrFile, "C:\\Document.txt");
    }
    return true;
}

BOOL GetSaveFileName(OPENFILENAME* lpofn) {
    if (!lpofn || !lpofn->lpstrFile || lpofn->nMaxFile == 0) return false;
    if (strlen(lpofn->lpstrFile) == 0) {
        strcpy(lpofn->lpstrFile, "C:\\Untitled.txt");
    }
    return true;
}
