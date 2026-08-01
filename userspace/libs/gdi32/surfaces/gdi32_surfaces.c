#include "../include/gdi32_api.h"

bool gdi32_surface_allocate_buffer(size_t width, size_t height, void** out_ptr, size_t* out_pitch) {
    if (!out_ptr || !out_pitch) return false;
    *out_ptr = NULL;
    *out_pitch = width * 4;
    return true;
}
