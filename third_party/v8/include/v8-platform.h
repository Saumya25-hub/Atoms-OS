/*
 * Copyright 2013 the V8 project authors. All rights reserved.
 * Copyright 2026 The V8 Authors / ATOMS OS Adaptation
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef INCLUDE_V8_PLATFORM_H_
#define INCLUDE_V8_PLATFORM_H_

#include <stddef.h>
#include <stdint.h>

namespace v8 {

class PageAllocator {
public:
    enum Permission {
        kNoAccess,
        kReadWrite,
        kReadExecute,
        kReadWriteExecute
    };

    virtual ~PageAllocator() = default;
    virtual size_t AllocatePageSize() = 0;
    virtual size_t CommitPageSize() = 0;
    virtual void* AllocatePages(void* address, size_t length, size_t alignment, Permission permissions) = 0;
    virtual bool FreePages(void* address, size_t length) = 0;
    virtual bool ReleasePages(void* address, size_t length, size_t new_length) = 0;
    virtual bool SetPermissions(void* address, size_t length, Permission permissions) = 0;
};

class Platform {
public:
    virtual ~Platform() = default;
    virtual PageAllocator* GetPageAllocator() = 0;
    virtual double MonotonicallyIncreasingTime() = 0;
    virtual double CurrentClockTimeMillis() = 0;
};

typedef bool (*EntropySource)(unsigned char* buffer, size_t length);

} // namespace v8

#endif // INCLUDE_V8_PLATFORM_H_
