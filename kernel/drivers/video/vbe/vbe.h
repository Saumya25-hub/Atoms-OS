#ifndef VBE_H
#define VBE_H

#include "bovisual/Include/bovisual_types.h"
#include "kernel/core/core_legacy/boot/include/boot_info.h"
#include "kernel/shell/console/console.h"

void vbe_init(boot_info_t* boot_info);
BVFramebuffer* vbe_get_framebuffer(void);
BackendDriver* vbe_get_console_backend(void);

// Double-buffer page-flip API
BVFramebuffer vbe_get_back_page(void);   // Get the hidden back page for rendering
BVFramebuffer* vbe_get_back_page_ptr(void);
BVFramebuffer* vbe_get_front_page_ptr(void);
void vbe_swap_page(void);                // Atomically flip display to back page

#endif // VBE_H
