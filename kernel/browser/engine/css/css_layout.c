#include "css_layout.h"
#include "kernel/wm/bwe/include/bwe.h"

extern void bwe_log(const char* level, const char* msg);

void ATRIX_CSSLayout_Init(void) {
    bwe_log("INFO", "ATRIX CSS Box Model & Flexbox Layout Subsystem Initialized");
}

void ATRIX_CSSLayout_ComputeBoxModel(CSSBoxModel* box, int32_t container_width, int32_t container_height) {
    if (!box) return;
    box->x = box->margin;
    box->y = box->margin;
    box->width = container_width - (box->margin * 2);
    box->height = container_height - (box->margin * 2);
}
