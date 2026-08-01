#include "../include/bosll_api.h"

extern void* kmalloc(size_t size);
extern void kfree(void* ptr);

void* BosAllocateHeap(size_t size) {
    if (size == 0) return NULL;
    return kmalloc(size);
}

bool BosFreeHeap(void* ptr) {
    if (!ptr) return false;
    kfree(ptr);
    return true;
}
