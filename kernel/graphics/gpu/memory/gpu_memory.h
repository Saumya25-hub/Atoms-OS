#ifndef BOS_GPU_MEMORY_H
#define BOS_GPU_MEMORY_H

#include "kernel/graphics/gpu/include/gpu.h"

void*            gpu_mem_alloc(size_t size);
void             gpu_mem_free(void* ptr);
bos_gpu_status_t gpu_mem_map_bar(bos_gpu_device_t* dev, uint8_t bar_index, void** out_virt);
bos_gpu_status_t gpu_mem_unmap_bar(bos_gpu_device_t* dev, uint8_t bar_index, void* virt);

#endif /* BOS_GPU_MEMORY_H */
