#include "../include/opengl32_api.h"

static uint64_t g_ctx_counter = 1;
static HGLRC g_current_hglrc = NULL;

HGLRC wglCreateContext(HANDLE hdc) {
    (void)hdc;
    return (HGLRC)(g_ctx_counter++);
}

BOOL wglDeleteContext(HGLRC hglrc) {
    if (hglrc == g_current_hglrc) g_current_hglrc = NULL;
    return TRUE;
}

BOOL wglMakeCurrent(HANDLE hdc, HGLRC hglrc) {
    (void)hdc;
    g_current_hglrc = hglrc;
    return TRUE;
}
