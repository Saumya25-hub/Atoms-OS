/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef MOJO_PUBLIC_C_SYSTEM_MESSAGE_PIPE_H_
#define MOJO_PUBLIC_C_SYSTEM_MESSAGE_PIPE_H_

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MojoCreateMessagePipeOptions {
    uint32_t struct_size;
    uint32_t flags;
} MojoCreateMessagePipeOptions;

typedef struct MojoWriteMessageOptions {
    uint32_t struct_size;
    uint32_t flags;
} MojoWriteMessageOptions;

typedef struct MojoReadMessageOptions {
    uint32_t struct_size;
    uint32_t flags;
} MojoReadMessageOptions;

// Mojo Core C ABI: Message Pipe Operations
MojoResult MojoCreateMessagePipe(
    const MojoCreateMessagePipeOptions* options,
    MojoHandle* out_handle_0,
    MojoHandle* out_handle_1);

MojoResult MojoWriteMessage(
    MojoHandle message_pipe_handle,
    const void* bytes,
    uint32_t num_bytes,
    const MojoHandle* handles,
    uint32_t num_handles,
    const MojoWriteMessageOptions* options);

MojoResult MojoReadMessage(
    MojoHandle message_pipe_handle,
    void* out_bytes,
    uint32_t* inout_num_bytes,
    MojoHandle* out_handles,
    uint32_t* inout_num_handles,
    const MojoReadMessageOptions* options);

MojoResult MojoClose(MojoHandle handle);

MojoResult MojoQueryHandleSignals(
    MojoHandle handle,
    MojoHandleSignals* out_signals);

#ifdef __cplusplus
}
#endif

#endif // MOJO_PUBLIC_C_SYSTEM_MESSAGE_PIPE_H_
