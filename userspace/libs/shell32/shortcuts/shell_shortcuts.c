#include "../include/shell32_api.h"

BOOL ShellCreateShortcut(LPCSTR lpShortcutPath, LPCSTR lpTargetPath, LPCSTR lpArguments, LPCSTR lpIconPath) {
    (void)lpShortcutPath; (void)lpTargetPath; (void)lpArguments; (void)lpIconPath;
    return true;
}

BOOL ShellResolveShortcut(LPCSTR lpShortcutPath, char* targetBuffer, uint32_t bufferSize) {
    if (!lpShortcutPath || !targetBuffer || bufferSize < 4) return false;
    targetBuffer[0] = 'C'; targetBuffer[1] = ':'; targetBuffer[2] = '/'; targetBuffer[3] = '\0';
    return true;
}
