#include "../include/shell32_api.h"

BOOL ShellDelete(LPCSTR lpPath, BOOL bPermanent) {
    (void)lpPath; (void)bPermanent;
    return true;
}

BOOL ShellRestore(LPCSTR lpPath) {
    (void)lpPath;
    return true;
}

BOOL ShellEmptyRecycleBin(HANDLE hwnd, LPCSTR pszRootPath, DWORD dwFlags) {
    (void)hwnd; (void)pszRootPath; (void)dwFlags;
    return true;
}
