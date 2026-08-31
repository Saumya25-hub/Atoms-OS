/*
 * Copyright (C) 2014 The Chromium Authors. All rights reserved.
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef MOJO_CORE_SHARED_BUFFER_H_
#define MOJO_CORE_SHARED_BUFFER_H_

#include "mojo/core/message_pipe.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

namespace mojo {
namespace core {

class SharedBufferDispatcher : public Dispatcher {
public:
    SharedBufferDispatcher(uint64_t num_bytes, uint32_t owner_pid);
    virtual ~SharedBufferDispatcher();

    MojoHandleType GetType() const override { return MOJO_HANDLE_TYPE_SHARED_BUFFER; }
    void Close() override;

    static SharedBufferDispatcher* Create(uint64_t num_bytes, uint32_t owner_pid);

    MojoResult Map(uint64_t offset, uint64_t num_bytes, void** out_ptr);
    MojoResult Unmap(void* ptr);
    SharedBufferDispatcher* Duplicate();

    uint64_t size() const { return size_; }
    uint32_t owner_pid() const { return owner_pid_; }
    void* buffer_ptr() const { return buffer_memory_; }

private:
    uint64_t size_;
    uint32_t owner_pid_;
    void* buffer_memory_;
    bool is_closed_;
    int map_count_;
};

} // namespace core
} // namespace mojo

#endif // MOJO_CORE_SHARED_BUFFER_H_
