#include "../include/gdi32_api.h"

bool gdi32_image_decode_png(const void* data, size_t size, HBITMAP* out_hbm) {
    (void)data; (void)size;
    if (!out_hbm) return false;
    *out_hbm = CreateBitmap(64, 64, 1, 32, NULL);
    return true;
}
