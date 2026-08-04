#ifndef SOFTWARE_BACKEND_H
#define SOFTWARE_BACKEND_H

#include "../render_backend.h"

extern const BOSPECTRA_RenderBackend g_software_render_backend;
bospectra_error_t bospectra_software_backend_get_pixels(void* surface_ctx, uint32_t** out_pixels, uint32_t* out_w, uint32_t* out_h);

#endif // SOFTWARE_BACKEND_H
