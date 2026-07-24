#include "abe_layout.h"
#include "kernel/wm/bwe/include/bwe.h"

extern void bwe_log(const char* level, const char* msg);

void ABE_Layout_Init(void) {
    bwe_log("INFO", "ABE Formatting Context & Flow Layout Subsystem Initialized");
}

void ABE_Layout_ComputeFlow(ABE_LayoutBox* box, int32_t viewport_w, int32_t viewport_h) {
    if (!box) return;

    box->x = box->margin;
    box->y = box->margin;
    box->width = viewport_w - (2 * box->margin);
    box->height = viewport_h - (2 * box->margin);
}
