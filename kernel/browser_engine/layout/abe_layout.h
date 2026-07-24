#ifndef ABE_LAYOUT_H
#define ABE_LAYOUT_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Viewport & Formatting Context Layout Engine
typedef struct {
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
    int32_t margin;
    int32_t padding;
    int32_t border;
} ABE_LayoutBox;

void ABE_Layout_Init(void);
void ABE_Layout_ComputeFlow(ABE_LayoutBox* box, int32_t viewport_w, int32_t viewport_h);

#ifdef __cplusplus
}
#endif

#endif // ABE_LAYOUT_H
