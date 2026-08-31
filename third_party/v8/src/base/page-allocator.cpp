/*
 * Copyright 2017 the V8 project authors. All rights reserved.
 * Copyright 2026 The V8 Authors / ATOMS OS Adaptation
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "page-allocator.h"
#include "userspace/runtime/c/include/sys/mman.h"
#include "userspace/runtime/c/include/unistd.h"

namespace v8 {
namespace base {

static int ToAtomsProt(v8::PageAllocator::Permission permissions) {
    switch (permissions) {
        case v8::PageAllocator::kNoAccess:
            return PROT_NONE;
        case v8::PageAllocator::kReadWrite:
            return PROT_READ | PROT_WRITE;
        case v8::PageAllocator::kReadExecute:
            return PROT_READ | PROT_EXEC;
        case v8::PageAllocator::kReadWriteExecute:
            return PROT_READ | PROT_WRITE | PROT_EXEC;
        default:
            return PROT_READ | PROT_WRITE;
    }
}

AtomsPageAllocator::AtomsPageAllocator() {
    page_size_ = 4096;
}

size_t AtomsPageAllocator::AllocatePageSize() {
    return page_size_;
}

size_t AtomsPageAllocator::CommitPageSize() {
    return page_size_;
}

void* AtomsPageAllocator::AllocatePages(void* address, size_t length, size_t alignment, Permission permissions) {
    (void)alignment;
    if (length == 0) return nullptr;

    // Round up length to page boundary
    size_t aligned_len = (length + page_size_ - 1) & ~(page_size_ - 1);
    int prot = ToAtomsProt(permissions);
    int flags = MAP_PRIVATE | MAP_ANONYMOUS;

    void* ptr = mmap(address, aligned_len, prot, flags, -1, 0);
    if (ptr == MAP_FAILED) {
        return nullptr;
    }
    return ptr;
}

bool AtomsPageAllocator::FreePages(void* address, size_t length) {
    if (!address || length == 0) return false;
    size_t aligned_len = (length + page_size_ - 1) & ~(page_size_ - 1);
    return munmap(address, aligned_len) == 0;
}

bool AtomsPageAllocator::ReleasePages(void* address, size_t length, size_t new_length) {
    if (!address || length <= new_length) return false;
    uint8_t* start = (uint8_t*)address + new_length;
    size_t excess = length - new_length;
    return munmap(start, excess) == 0;
}

bool AtomsPageAllocator::SetPermissions(void* address, size_t length, Permission permissions) {
    if (!address || length == 0) return false;
    size_t aligned_len = (length + page_size_ - 1) & ~(page_size_ - 1);
    int prot = ToAtomsProt(permissions);
    return mprotect(address, aligned_len, prot) == 0;
}

} // namespace base
} // namespace v8
