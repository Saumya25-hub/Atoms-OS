/*
 * Copyright 2021 the V8 project authors. All rights reserved.
 * Copyright 2026 The V8 Authors / ATOMS OS Adaptation
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef INCLUDE_V8_ISOLATE_H_
#define INCLUDE_V8_ISOLATE_H_

#include <stddef.h>
#include <stdint.h>

namespace v8 {

class Platform;

class Isolate {
public:
    struct CreateParams {
        CreateParams() : entry_hook(nullptr), code_event_handler(nullptr),
                         constraints(), array_buffer_allocator(nullptr) {}
        void* entry_hook;
        void* code_event_handler;
        struct {
            size_t max_old_generation_size_in_bytes = 128 * 1024 * 1024;
            size_t max_young_generation_size_in_bytes = 16 * 1024 * 1024;
        } constraints;
        void* array_buffer_allocator;
    };

    class Scope {
    public:
        explicit Scope(Isolate* isolate);
        ~Scope();
    private:
        Isolate* isolate_;
    };

    static Isolate* New(const CreateParams& params);
    static Isolate* GetCurrent();

    void Enter();
    void Exit();
    void Dispose();

    void LowMemoryNotification();
    void RequestGarbageCollectionForTesting();

    void* GetInternalHeap();
};

} // namespace v8

#endif // INCLUDE_V8_ISOLATE_H_
