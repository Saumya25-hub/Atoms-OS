#include "../include/shell32_api.h"

BOOL ShellRegisterFileAssociation(LPCSTR lpExtension, LPCSTR lpAppPath) {
    (void)lpExtension; (void)lpAppPath;
    return true;
}

BOOL ShellGetFileAssociation(LPCSTR lpExtension, char* appPathBuffer, uint32_t bufferSize) {
    if (!lpExtension || !appPathBuffer || bufferSize < 12) return false;
    appPathBuffer[0] = 'N'; appPathBuffer[1] = 'o'; appPathBuffer[2] = 't'; appPathBuffer[3] = 'e'; appPathBuffer[4] = 'p'; appPathBuffer[5] = 'a'; appPathBuffer[6] = 'd'; appPathBuffer[7] = '.'; appPathBuffer[8] = 'e'; appPathBuffer[9] = 'x'; appPathBuffer[10] = 'e'; appPathBuffer[11] = '\0';
    return true;
}
