#include "../include/opengl32_api.h"

void* wglGetProcAddress(LPCSTR lpszProc) {
    if (!lpszProc) return NULL;
    if (lpszProc[0] == 'w' && lpszProc[1] == 'g') return (void*)wglSwapBuffers;
    return (void*)glDrawArrays;
}
