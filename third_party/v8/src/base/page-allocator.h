/*
 * Copyright 2017 the V8 project authors. All rights reserved.
 * Copyright 2026 The V8 Authors / ATOMS OS Adaptation
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef V8_BASE_PAGE_ALLOCATOR_H_
#define V8_BASE_PAGE_ALLOCATOR_H_

#include "third_party/v8/include/v8-platform.h"

namespace v8 {
namespace base {

class AtomsPageAllocator : public v8::PageAllocator {
public:
    AtomsPageAllocator();
    ~AtomsPageAllocator() override = default;

    size_t AllocatePageSize() override;
    size_t CommitPageSize() override;
    void* AllocatePages(void* address, size_t length, size_t alignment, Permission permissions) override;
    bool FreePages(void* address, size_t length) override;
    bool ReleasePages(void* address, size_t length, size_t new_length) override;
    bool SetPermissions(void* address, size_t length, Permission permissions) override;

private:
    size_t page_size_;
};

} // namespace base
} // namespace v8

#endif // V8_BASE_PAGE_ALLOCATOR_H_
