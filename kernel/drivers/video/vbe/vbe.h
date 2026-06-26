#ifndef VBE_H
#define VBE_H

#include "bovisual/Include/bovisual_types.h"
#include "kernel/boot/include/boot_info.h"

void vbe_init(boot_info_t* boot_info);
BVFramebuffer* vbe_get_framebuffer(void);

#endif // VBE_H
