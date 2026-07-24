#ifndef ABE_RENDER_H
#define ABE_RENDER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Software & Hardware Rasterization Paint Subsystem
typedef struct {
    uint32_t pixels_painted;
    uint32_t text_elements_drawn;
    uint32_t borders_drawn;
} ABE_PaintStats;

void           ABE_Render_Init(void);
ABE_PaintStats ABE_Render_PaintCanvas(void* buffer, int32_t width, int32_t height, uint32_t bg_color);

#ifdef __cplusplus
}
#endif

#endif // ABE_RENDER_H
