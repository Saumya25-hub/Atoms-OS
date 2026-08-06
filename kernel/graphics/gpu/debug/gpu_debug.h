#ifndef BOS_GPU_DEBUG_H
#define BOS_GPU_DEBUG_H

#include "kernel/graphics/gpu/include/gpu.h"

void gpu_log_info(const char* msg);
void gpu_log_warn(const char* msg);
void gpu_log_err(const char* msg);
void gpu_dump_device_info(const bos_gpu_device_t* dev);
void gpu_dump_capabilities(uint64_t caps);

#endif /* BOS_GPU_DEBUG_H */
