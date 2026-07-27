#include "kernel/shell/rook/include/spinner.h"
#include "kernel/ame/include/ame.h"

/*
 * 🌀 ATOMS OS Legacy Spinner Wrapper
 * Forwards legacy calls to ATOMS Motion Engine (AME) Spinner Module.
 */

void spinner_init(spinner_t* sp, int cx, int cy, int radius, int num_dots) {
    (void)sp;
    AME_Spinner_Init(AME_GetBootSpinner(), cx, cy, radius, num_dots);
}

void spinner_set_position(spinner_t* sp, int cx, int cy) {
    (void)sp;
    AME_Spinner_SetPosition(AME_GetBootSpinner(), cx, cy);
}

void spinner_update(spinner_t* sp, uint64_t delta_ms) {
    (void)sp;
    AME_Update(delta_ms);
}

void spinner_render(const spinner_t* sp, uint32_t* framebuffer, uint32_t fb_width, uint32_t fb_height, uint32_t fb_stride) {
    (void)sp;
    AME_Spinner_Render(AME_GetBootSpinner(), framebuffer, fb_width, fb_height, fb_stride);
}
