/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef MOJO_CORE_HANDLE_TABLE_H_
#define MOJO_CORE_HANDLE_TABLE_H_

#include "mojo/public/c/system/types.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

namespace mojo {
namespace core {

class Dispatcher;

struct HandleEntry {
    MojoHandle handle;
    MojoHandleType type;
    MojoHandleRights rights;
    Dispatcher* dispatcher;
    uint32_t owner_pid;
    bool is_valid;
};

#define MOJO_MAX_HANDLES 1024

class HandleTable {
public:
    HandleTable();
    ~HandleTable();

    static HandleTable* GetInstance();

    MojoResult AddDispatcher(Dispatcher* dispatcher, MojoHandleType type, MojoHandleRights rights, uint32_t owner_pid, MojoHandle* out_handle);
    Dispatcher* GetDispatcher(MojoHandle handle, MojoHandleType expected_type, MojoHandleRights required_rights);
    MojoResult CloseHandle(MojoHandle handle);
    MojoResult TransferHandle(MojoHandle handle, uint32_t target_pid, MojoHandle* out_new_handle);

    void CloseAllHandlesForProcess(uint32_t pid);
    size_t active_handle_count() const { return active_count_; }

private:
    HandleEntry entries_[MOJO_MAX_HANDLES];
    uint32_t next_handle_id_;
    size_t active_count_;
};

} // namespace core
} // namespace mojo

#endif // MOJO_CORE_HANDLE_TABLE_H_
