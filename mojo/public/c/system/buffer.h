/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef MOJO_PUBLIC_C_SYSTEM_BUFFER_H_
#define MOJO_PUBLIC_C_SYSTEM_BUFFER_H_

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MojoCreateSharedBufferOptions {
    uint32_t struct_size;
    uint32_t flags;
} MojoCreateSharedBufferOptions;

typedef struct MojoMapBufferOptions {
    uint32_t struct_size;
    uint32_t flags;
} MojoMapBufferOptions;

typedef struct MojoDuplicateBufferHandleOptions {
    uint32_t struct_size;
    uint32_t flags;
} MojoDuplicateBufferHandleOptions;

// Mojo Core C ABI: Shared Buffer Operations
MojoResult MojoCreateSharedBuffer(
    uint64_t num_bytes,
    const MojoCreateSharedBufferOptions* options,
    MojoHandle* out_shared_buffer_handle);

MojoResult MojoMapBuffer(
    MojoHandle shared_buffer_handle,
    uint64_t offset,
    uint64_t num_bytes,
    const MojoMapBufferOptions* options,
    void** out_buffer_ptr);

MojoResult MojoUnmapBuffer(void* buffer_ptr);

MojoResult MojoDuplicateBufferHandle(
    MojoHandle shared_buffer_handle,
    const MojoDuplicateBufferHandleOptions* options,
    MojoHandle* out_new_handle);

#ifdef __cplusplus
}
#endif

#endif // MOJO_PUBLIC_C_SYSTEM_BUFFER_H_
