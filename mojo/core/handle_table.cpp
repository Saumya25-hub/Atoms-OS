/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "handle_table.h"
#include "message_pipe.h"
#include "shared_buffer.h"

namespace mojo {
namespace core {

static HandleTable* s_handle_table_instance = nullptr;

HandleTable::HandleTable()
    : next_handle_id_(1),
      active_count_(0) {
    for (uint32_t i = 0; i < MOJO_MAX_HANDLES; i++) {
        entries_[i].handle = MOJO_HANDLE_INVALID;
        entries_[i].type = MOJO_HANDLE_TYPE_INVALID;
        entries_[i].rights = MOJO_HANDLE_RIGHT_NONE;
        entries_[i].dispatcher = nullptr;
        entries_[i].owner_pid = 0;
        entries_[i].is_valid = false;
    }
}

HandleTable::~HandleTable() {
    for (uint32_t i = 0; i < MOJO_MAX_HANDLES; i++) {
        if (entries_[i].is_valid && entries_[i].dispatcher) {
            entries_[i].dispatcher->Close();
            delete entries_[i].dispatcher;
            entries_[i].dispatcher = nullptr;
        }
    }
}

HandleTable* HandleTable::GetInstance() {
    if (!s_handle_table_instance) {
        s_handle_table_instance = new HandleTable();
    }
    return s_handle_table_instance;
}

MojoResult HandleTable::AddDispatcher(Dispatcher* dispatcher, MojoHandleType type, MojoHandleRights rights, uint32_t owner_pid, MojoHandle* out_handle) {
    if (!dispatcher || !out_handle) return MOJO_RESULT_INVALID_ARGUMENT;
    if (active_count_ >= MOJO_MAX_HANDLES) return MOJO_RESULT_RESOURCE_EXHAUSTED;

    for (uint32_t i = 0; i < MOJO_MAX_HANDLES; i++) {
        uint32_t slot = (next_handle_id_ + i) % MOJO_MAX_HANDLES;
        if (!entries_[slot].is_valid) {
            MojoHandle h = (MojoHandle)(slot + 1);
            entries_[slot].handle = h;
            entries_[slot].type = type;
            entries_[slot].rights = rights;
            entries_[slot].dispatcher = dispatcher;
            entries_[slot].owner_pid = owner_pid;
            entries_[slot].is_valid = true;
            active_count_++;
            next_handle_id_ = (slot + 1) % MOJO_MAX_HANDLES;
            *out_handle = h;
            return MOJO_RESULT_OK;
        }
    }

    return MOJO_RESULT_RESOURCE_EXHAUSTED;
}

Dispatcher* HandleTable::GetDispatcher(MojoHandle handle, MojoHandleType expected_type, MojoHandleRights required_rights) {
    if (handle == MOJO_HANDLE_INVALID) return nullptr;
    uint32_t slot = (uint32_t)(handle - 1);
    if (slot >= MOJO_MAX_HANDLES) return nullptr;

    if (!entries_[slot].is_valid || entries_[slot].handle != handle) {
        return nullptr;
    }

    if (expected_type != MOJO_HANDLE_TYPE_INVALID && entries_[slot].type != expected_type) {
        return nullptr;
    }

    if (required_rights != MOJO_HANDLE_RIGHT_NONE && (entries_[slot].rights & required_rights) != required_rights) {
        return nullptr;
    }

    return entries_[slot].dispatcher;
}

MojoResult HandleTable::CloseHandle(MojoHandle handle) {
    if (handle == MOJO_HANDLE_INVALID) return MOJO_RESULT_INVALID_ARGUMENT;
    uint32_t slot = (uint32_t)(handle - 1);
    if (slot >= MOJO_MAX_HANDLES) return MOJO_RESULT_INVALID_ARGUMENT;

    if (!entries_[slot].is_valid || entries_[slot].handle != handle) {
        return MOJO_RESULT_INVALID_ARGUMENT;
    }

    Dispatcher* d = entries_[slot].dispatcher;
    entries_[slot].is_valid = false;
    entries_[slot].handle = MOJO_HANDLE_INVALID;
    entries_[slot].dispatcher = nullptr;
    if (active_count_ > 0) active_count_--;

    if (d) {
        d->Close();
        delete d;
    }

    return MOJO_RESULT_OK;
}

MojoResult HandleTable::TransferHandle(MojoHandle handle, uint32_t target_pid, MojoHandle* out_new_handle) {
    if (handle == MOJO_HANDLE_INVALID || !out_new_handle) return MOJO_RESULT_INVALID_ARGUMENT;
    uint32_t slot = (uint32_t)(handle - 1);
    if (slot >= MOJO_MAX_HANDLES) return MOJO_RESULT_INVALID_ARGUMENT;

    if (!entries_[slot].is_valid || entries_[slot].handle != handle) {
        return MOJO_RESULT_INVALID_ARGUMENT;
    }

    if (!(entries_[slot].rights & MOJO_HANDLE_RIGHT_TRANSFER)) {
        return MOJO_RESULT_PERMISSION_DENIED;
    }

    Dispatcher* d = entries_[slot].dispatcher;
    MojoHandleType type = entries_[slot].type;
    MojoHandleRights rights = entries_[slot].rights;

    // Invalidate old handle
    entries_[slot].is_valid = false;
    entries_[slot].handle = MOJO_HANDLE_INVALID;
    entries_[slot].dispatcher = nullptr;
    if (active_count_ > 0) active_count_--;

    // Re-register under target PID
    if (type == MOJO_HANDLE_TYPE_MESSAGE_PIPE) {
        MessagePipeDispatcher* mp = static_cast<MessagePipeDispatcher*>(d);
        mp->set_owner_pid(target_pid);
    }

    return AddDispatcher(d, type, rights, target_pid, out_new_handle);
}

void HandleTable::CloseAllHandlesForProcess(uint32_t pid) {
    for (uint32_t i = 0; i < MOJO_MAX_HANDLES; i++) {
        if (entries_[i].is_valid && entries_[i].owner_pid == pid) {
            CloseHandle(entries_[i].handle);
        }
    }
}

} // namespace core
} // namespace mojo
