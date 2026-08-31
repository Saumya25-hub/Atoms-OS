/*
 * Copyright 2013 the V8 project authors. All rights reserved.
 * Copyright 2026 The V8 Authors / ATOMS OS Adaptation
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "platform.h"
#include "userspace/runtime/c/include/time.h"
#include "userspace/runtime/c/include/stdlib.h"

namespace v8 {
namespace base {

AtomsDefaultPlatform::AtomsDefaultPlatform() {}

v8::PageAllocator* AtomsDefaultPlatform::GetPageAllocator() {
    return &page_allocator_;
}

double AtomsDefaultPlatform::MonotonicallyIncreasingTime() {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        return (double)ts.tv_sec + ((double)ts.tv_nsec / 1e9);
    }
    return 0.0;
}

double AtomsDefaultPlatform::CurrentClockTimeMillis() {
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == 0) {
        return ((double)ts.tv_sec * 1000.0) + ((double)ts.tv_nsec / 1e6);
    }
    return 0.0;
}

} // namespace base

static bool AtomsEntropySource(unsigned char* buffer, size_t length) {
    if (!buffer || length == 0) return false;
    for (size_t i = 0; i < length; i += 8) {
        uint64_t val = 0;
        uint8_t ok = 0;
        __asm__ volatile("rdrand %0; setc %1" : "=r"(val), "=qm"(ok));
        if (!ok) {
            // Fallback to rdtsc jitter
            uint64_t lo, hi;
            __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
            val = (hi << 32) | lo;
        }
        size_t copy_len = (length - i) < 8 ? (length - i) : 8;
        for (size_t k = 0; k < copy_len; k++) {
            buffer[i + k] = (uint8_t)((val >> (k * 8)) & 0xFF);
        }
    }
    return true;
}

} // namespace v8
