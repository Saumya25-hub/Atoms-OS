#include "kernel/graphics/gpu/memory/gpu_memory.h"
#include "kernel/graphics/gpu/debug/gpu_debug.h"

extern void* kmalloc(uint32_t size);
extern void  kfree(void* ptr);

void* gpu_mem_alloc(size_t size) {
    if (size == 0) return NULL;
    return kmalloc((uint32_t)size);
}

void gpu_mem_free(void* ptr) {
    if (ptr) {
        kfree(ptr);
    }
}

bos_gpu_status_t gpu_mem_map_bar(bos_gpu_device_t* dev, uint8_t bar_index, void** out_virt) {
    if (!dev || bar_index >= 6 || !out_virt) {
        return BOS_GPU_ERR_INVALID_PARAM;
    }
    if (dev->bars[bar_index].type == BOS_GPU_BAR_TYPE_UNUSED) {
        return BOS_GPU_ERR_NOT_SUPPORTED;
    }

    /* Identity map or direct BAR base address access for identity kernel space */
    *out_virt = (void*)(uintptr_t)dev->bars[bar_index].base_address;
    return BOS_GPU_OK;
}

bos_gpu_status_t gpu_mem_unmap_bar(bos_gpu_device_t* dev, uint8_t bar_index, void* virt) {
    (void)dev;
    (void)bar_index;
    (void)virt;
    return BOS_GPU_OK;
}
