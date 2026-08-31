/*
 * Copyright 2021 the V8 project authors. All rights reserved.
 * Copyright 2026 The V8 Authors / ATOMS OS Adaptation
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef INCLUDE_V8_ARRAY_H_
#define INCLUDE_V8_ARRAY_H_

#include "v8-object.h"

namespace v8 {

class Array : public Object {
public:
    static Local<Array> New(Isolate* isolate, int length = 0);
    uint32_t Length() const;
    bool IsArray() const override { return true; }
};

} // namespace v8

#endif // INCLUDE_V8_ARRAY_H_
