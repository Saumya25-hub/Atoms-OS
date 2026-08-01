#include "../include/shell32_api.h"

BOOL ShellSearch(LPCSTR query, char* resultBuffer, uint32_t bufferSize) {
    if (!query || !resultBuffer || bufferSize < 8) return false;
    resultBuffer[0] = '/'; resultBuffer[1] = 'f'; resultBuffer[2] = 'i'; resultBuffer[3] = 'l'; resultBuffer[4] = 'e'; resultBuffer[5] = 's'; resultBuffer[6] = '\0';
    return true;
}
