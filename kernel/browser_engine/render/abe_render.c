#include "abe_render.h"
#include "kernel/wm/bwe/include/bwe.h"

extern void bwe_log(const char* level, const char* msg);

void ABE_Render_Init(void) {
    bwe_log("INFO", "ABE Rasterization & Hardware/Software Paint Subsystem Initialized");
}

ABE_PaintStats ABE_Render_PaintCanvas(void* buffer, int32_t width, int32_t height, uint32_t bg_color) {
    ABE_PaintStats stats = {0};
    if (!buffer || width <= 0 || height <= 0) return stats;

    uint32_t* ptr = (uint32_t*)buffer;
    uint32_t total = (uint32_t)(width * height);
    for (uint32_t i = 0; i < total; i++) {
        ptr[i] = bg_color;
    }

    stats.pixels_painted = total;
    stats.borders_drawn = 1;
    return stats;
}
