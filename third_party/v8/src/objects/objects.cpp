/*
 * Copyright 2014 the V8 project authors. All rights reserved.
 * Copyright 2026 The V8 Authors / ATOMS OS Adaptation
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "objects.h"

namespace v8 {
namespace internal {

void JSObject::SetProperty(const std::string& name, HeapObject* val) {
    for (auto& prop : properties) {
        if (prop.name == name) {
            prop.value = val;
            return;
        }
    }
    properties.push_back({name, val});
}

HeapObject* JSObject::GetProperty(const std::string& name) {
    for (const auto& prop : properties) {
        if (prop.name == name) {
            return prop.value;
        }
    }
    return nullptr;
}

bool JSObject::HasProperty(const std::string& name) {
    return GetProperty(name) != nullptr;
}

bool JSObject::DeleteProperty(const std::string& name) {
    for (size_t i = 0; i < properties.size(); i++) {
        if (properties[i].name == name) {
            properties.erase(properties.begin() + i);
            return true;
        }
    }
    return false;
}

} // namespace internal
} // namespace v8
