#ifndef ATRIX_CSS_LAYOUT_H
#define ATRIX_CSS_LAYOUT_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// ATRIX Box Model & Flex Layout Subsystem
// ============================================================

typedef struct {
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
    int32_t margin;
    int32_t padding;
    int32_t border;
} CSSBoxModel;

void ATRIX_CSSLayout_Init(void);
void ATRIX_CSSLayout_ComputeBoxModel(CSSBoxModel* box, int32_t container_width, int32_t container_height);

#ifdef __cplusplus
}
#endif

#endif // ATRIX_CSS_LAYOUT_H
