// Engine 19: GPU Memory Manager (VRAM Pool)
#include "../include/agp_api.h"

extern void* kmalloc(size_t size);
extern void kfree(void* ptr);

void* AGP_VRAMAlloc(size_t size) {
    return kmalloc(size);
}

void AGP_VRAMFree(void* ptr) {
    kfree(ptr);
}
