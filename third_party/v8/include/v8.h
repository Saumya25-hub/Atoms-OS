/*
 * Copyright 2014 the V8 project authors. All rights reserved.
 * Copyright 2026 The V8 Authors / ATOMS OS Adaptation
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef INCLUDE_V8_H_
#define INCLUDE_V8_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "v8-platform.h"
#include "v8-isolate.h"
#include "v8-context.h"
#include "v8-value.h"
#include "v8-primitive.h"
#include "v8-object.h"
#include "v8-array.h"
#include "v8-function.h"
#include "v8-script.h"
#include "v8-handle-scope.h"
#include "v8-exception.h"

namespace v8 {

class V8 {
public:
    static void InitializePlatform(Platform* platform);
    static void Initialize();
    static bool Dispose();
    static void DisposePlatform();
    static Platform* GetCurrentPlatform();

    static void SetEntropySource(EntropySource entropy_source);
    static const char* GetVersion();
};

} // namespace v8

#endif // INCLUDE_V8_H_
