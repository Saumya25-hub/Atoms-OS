// Engine 15: EGL Compatibility Layer
#include "../include/agp_api.h"

void* eglGetDisplay(void* display_id) {
    (void)display_id;
    return (void*)0x1000;
}

int eglInitialize(void* dpy, int* major, int* minor) {
    (void)dpy;
    if (major) *major = 1;
    if (minor) *minor = 4;
    return 1;
}
