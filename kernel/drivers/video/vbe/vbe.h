#ifndef VBE_H
#define VBE_H

#include "bovisual/Include/bovisual_types.h"
#include "kernel/core/core_legacy/boot/include/boot_info.h"

void vbe_init(boot_info_t* boot_info);
BVFramebuffer* vbe_get_framebuffer(void);

// Double-buffer page-flip API
BVFramebuffer vbe_get_back_page(void);   // Get the hidden back page for rendering
void vbe_swap_page(void);                // Atomically flip display to back page

#endif // VBE_H
