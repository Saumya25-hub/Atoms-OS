/*
 * Copyright 2013 the V8 project authors. All rights reserved.
 * Copyright 2026 The V8 Authors / ATOMS OS Adaptation
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef V8_BASE_PLATFORM_PLATFORM_H_
#define V8_BASE_PLATFORM_PLATFORM_H_

#include "third_party/v8/include/v8-platform.h"
#include "third_party/v8/src/base/page-allocator.h"

namespace v8 {
namespace base {

class AtomsDefaultPlatform : public v8::Platform {
public:
    AtomsDefaultPlatform();
    ~AtomsDefaultPlatform() override = default;

    v8::PageAllocator* GetPageAllocator() override;
    double MonotonicallyIncreasingTime() override;
    double CurrentClockTimeMillis() override;

private:
    AtomsPageAllocator page_allocator_;
};

} // namespace base
} // namespace v8

#endif // V8_BASE_PLATFORM_PLATFORM_H_
