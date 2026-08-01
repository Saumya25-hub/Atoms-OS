#include "../include/opengl32_api.h"

int wglChoosePixelFormat(HANDLE hdc, const PIXELFORMATDESCRIPTOR* ppfd) {
    (void)hdc; (void)ppfd;
    return 1; // Default pixel format index 1
}

BOOL wglSetPixelFormat(HANDLE hdc, int iPixelFormat, const PIXELFORMATDESCRIPTOR* ppfd) {
    (void)hdc; (void)iPixelFormat; (void)ppfd;
    return TRUE;
}
