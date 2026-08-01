#include "../include/opengl32_api.h"

BOOL wglSwapBuffers(HANDLE hdc) {
    (void)hdc;
    return TRUE;
}

void glFlush(void) {}
void glFinish(void) {}
