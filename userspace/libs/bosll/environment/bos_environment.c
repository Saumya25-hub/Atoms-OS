#include "../include/bosll_api.h"

const char* BosGetEnvironment(const char* varName) {
    if (!varName) return NULL;
    if (varName[0] == 'P' && varName[1] == 'A') return "C:/System32;C:/Bin";
    return "ATOMS_OS";
}
