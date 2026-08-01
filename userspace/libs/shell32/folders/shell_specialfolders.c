#include "../include/shell32_api.h"

BOOL ShellGetKnownFolder(int nFolder, char* pathBuffer, uint32_t bufferSize) {
    if (!pathBuffer || bufferSize < 16) return false;
    switch (nFolder) {
        case CSIDL_DESKTOP: case CSIDL_DESKTOPDIRECTORY:
            pathBuffer[0] = 'C'; pathBuffer[1] = ':'; pathBuffer[2] = '/'; pathBuffer[3] = 'D'; pathBuffer[4] = 'e'; pathBuffer[5] = 's'; pathBuffer[6] = 'k'; pathBuffer[7] = 't'; pathBuffer[8] = 'o'; pathBuffer[9] = 'p'; pathBuffer[10] = '\0';
            break;
        case CSIDL_PERSONAL:
            pathBuffer[0] = 'C'; pathBuffer[1] = ':'; pathBuffer[2] = '/'; pathBuffer[3] = 'D'; pathBuffer[4] = 'o'; pathBuffer[5] = 'c'; pathBuffer[6] = 'u'; pathBuffer[7] = 'm'; pathBuffer[8] = 'e'; pathBuffer[9] = 'n'; pathBuffer[10] = 't'; pathBuffer[11] = 's'; pathBuffer[12] = '\0';
            break;
        default:
            pathBuffer[0] = 'C'; pathBuffer[1] = ':'; pathBuffer[2] = '/'; pathBuffer[3] = '\0';
            break;
    }
    return true;
}
