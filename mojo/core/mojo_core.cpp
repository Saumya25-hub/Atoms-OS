/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "mojo/public/c/system/message_pipe.h"
#include "mojo/public/c/system/buffer.h"
#include "handle_table.h"
#include "message_pipe.h"
#include "shared_buffer.h"

extern "C" {

MojoResult MojoCreateMessagePipe(
    const MojoCreateMessagePipeOptions* options,
    MojoHandle* out_handle_0,
    MojoHandle* out_handle_1) {
    (void)options;
    if (!out_handle_0 || !out_handle_1) return MOJO_RESULT_INVALID_ARGUMENT;

    mojo::core::MessagePipeDispatcher* d0 = nullptr;
    mojo::core::MessagePipeDispatcher* d1 = nullptr;

    mojo::core::MessagePipeDispatcher::CreatePair(0, 0, &d0, &d1);
    if (!d0 || !d1) return MOJO_RESULT_RESOURCE_EXHAUSTED;

    mojo::core::HandleTable* ht = mojo::core::HandleTable::GetInstance();

    MojoHandleRights rights = MOJO_HANDLE_RIGHT_READ | MOJO_HANDLE_RIGHT_WRITE | MOJO_HANDLE_RIGHT_TRANSFER;
    MojoResult r0 = ht->AddDispatcher(d0, MOJO_HANDLE_TYPE_MESSAGE_PIPE, rights, 0, out_handle_0);
    if (r0 != MOJO_RESULT_OK) {
        delete d0;
        delete d1;
        return r0;
    }

    MojoResult r1 = ht->AddDispatcher(d1, MOJO_HANDLE_TYPE_MESSAGE_PIPE, rights, 0, out_handle_1);
    if (r1 != MOJO_RESULT_OK) {
        ht->CloseHandle(*out_handle_0);
        delete d1;
        return r1;
    }

    return MOJO_RESULT_OK;
}

MojoResult MojoWriteMessage(
    MojoHandle message_pipe_handle,
    const void* bytes,
    uint32_t num_bytes,
    const MojoHandle* handles,
    uint32_t num_handles,
    const MojoWriteMessageOptions* options) {
    (void)options;
    mojo::core::HandleTable* ht = mojo::core::HandleTable::GetInstance();
    mojo::core::Dispatcher* d = ht->GetDispatcher(message_pipe_handle, MOJO_HANDLE_TYPE_MESSAGE_PIPE, MOJO_HANDLE_RIGHT_WRITE);
    if (!d) return MOJO_RESULT_INVALID_ARGUMENT;

    mojo::core::MessagePipeDispatcher* mp = static_cast<mojo::core::MessagePipeDispatcher*>(d);
    return mp->WriteMessage(bytes, num_bytes, handles, num_handles);
}

MojoResult MojoReadMessage(
    MojoHandle message_pipe_handle,
    void* out_bytes,
    uint32_t* inout_num_bytes,
    MojoHandle* out_handles,
    uint32_t* inout_num_handles,
    const MojoReadMessageOptions* options) {
    (void)options;
    mojo::core::HandleTable* ht = mojo::core::HandleTable::GetInstance();
    mojo::core::Dispatcher* d = ht->GetDispatcher(message_pipe_handle, MOJO_HANDLE_TYPE_MESSAGE_PIPE, MOJO_HANDLE_RIGHT_READ);
    if (!d) return MOJO_RESULT_INVALID_ARGUMENT;

    mojo::core::MessagePipeDispatcher* mp = static_cast<mojo::core::MessagePipeDispatcher*>(d);
    return mp->ReadMessage(out_bytes, inout_num_bytes, out_handles, inout_num_handles);
}

MojoResult MojoClose(MojoHandle handle) {
    mojo::core::HandleTable* ht = mojo::core::HandleTable::GetInstance();
    return ht->CloseHandle(handle);
}

MojoResult MojoQueryHandleSignals(
    MojoHandle handle,
    MojoHandleSignals* out_signals) {
    if (!out_signals) return MOJO_RESULT_INVALID_ARGUMENT;
    mojo::core::HandleTable* ht = mojo::core::HandleTable::GetInstance();
    mojo::core::Dispatcher* d = ht->GetDispatcher(handle, MOJO_HANDLE_TYPE_MESSAGE_PIPE, MOJO_HANDLE_RIGHT_NONE);
    if (!d) return MOJO_RESULT_INVALID_ARGUMENT;

    mojo::core::MessagePipeDispatcher* mp = static_cast<mojo::core::MessagePipeDispatcher*>(d);
    *out_signals = mp->QuerySignals();
    return MOJO_RESULT_OK;
}

MojoResult MojoCreateSharedBuffer(
    uint64_t num_bytes,
    const MojoCreateSharedBufferOptions* options,
    MojoHandle* out_shared_buffer_handle) {
    (void)options;
    if (!out_shared_buffer_handle || num_bytes == 0) return MOJO_RESULT_INVALID_ARGUMENT;

    mojo::core::SharedBufferDispatcher* sb = mojo::core::SharedBufferDispatcher::Create(num_bytes, 0);
    if (!sb) return MOJO_RESULT_RESOURCE_EXHAUSTED;

    mojo::core::HandleTable* ht = mojo::core::HandleTable::GetInstance();
    MojoHandleRights rights = MOJO_HANDLE_RIGHT_READ | MOJO_HANDLE_RIGHT_WRITE | MOJO_HANDLE_RIGHT_TRANSFER |
                              MOJO_HANDLE_RIGHT_MAP_READ | MOJO_HANDLE_RIGHT_MAP_WRITE | MOJO_HANDLE_RIGHT_DUPLICATE;

    return ht->AddDispatcher(sb, MOJO_HANDLE_TYPE_SHARED_BUFFER, rights, 0, out_shared_buffer_handle);
}

MojoResult MojoMapBuffer(
    MojoHandle shared_buffer_handle,
    uint64_t offset,
    uint64_t num_bytes,
    const MojoMapBufferOptions* options,
    void** out_buffer_ptr) {
    (void)options;
    if (!out_buffer_ptr) return MOJO_RESULT_INVALID_ARGUMENT;

    mojo::core::HandleTable* ht = mojo::core::HandleTable::GetInstance();
    mojo::core::Dispatcher* d = ht->GetDispatcher(shared_buffer_handle, MOJO_HANDLE_TYPE_SHARED_BUFFER, MOJO_HANDLE_RIGHT_MAP_READ);
    if (!d) return MOJO_RESULT_INVALID_ARGUMENT;

    mojo::core::SharedBufferDispatcher* sb = static_cast<mojo::core::SharedBufferDispatcher*>(d);
    return sb->Map(offset, num_bytes, out_buffer_ptr);
}

MojoResult MojoUnmapBuffer(void* buffer_ptr) {
    if (!buffer_ptr) return MOJO_RESULT_INVALID_ARGUMENT;
    return MOJO_RESULT_OK;
}

MojoResult MojoDuplicateBufferHandle(
    MojoHandle shared_buffer_handle,
    const MojoDuplicateBufferHandleOptions* options,
    MojoHandle* out_new_handle) {
    (void)options;
    if (!out_new_handle) return MOJO_RESULT_INVALID_ARGUMENT;

    mojo::core::HandleTable* ht = mojo::core::HandleTable::GetInstance();
    mojo::core::Dispatcher* d = ht->GetDispatcher(shared_buffer_handle, MOJO_HANDLE_TYPE_SHARED_BUFFER, MOJO_HANDLE_RIGHT_DUPLICATE);
    if (!d) return MOJO_RESULT_INVALID_ARGUMENT;

    mojo::core::SharedBufferDispatcher* sb = static_cast<mojo::core::SharedBufferDispatcher*>(d);
    mojo::core::SharedBufferDispatcher* dup = sb->Duplicate();
    if (!dup) return MOJO_RESULT_RESOURCE_EXHAUSTED;

    MojoHandleRights rights = MOJO_HANDLE_RIGHT_READ | MOJO_HANDLE_RIGHT_WRITE | MOJO_HANDLE_RIGHT_TRANSFER |
                              MOJO_HANDLE_RIGHT_MAP_READ | MOJO_HANDLE_RIGHT_MAP_WRITE | MOJO_HANDLE_RIGHT_DUPLICATE;

    return ht->AddDispatcher(dup, MOJO_HANDLE_TYPE_SHARED_BUFFER, rights, 0, out_new_handle);
}

} // extern "C"
