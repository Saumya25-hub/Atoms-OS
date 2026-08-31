/*
 * Copyright 2014 the V8 project authors. All rights reserved.
 * Copyright 2026 The V8 Authors / ATOMS OS Adaptation
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "heap.h"
#include "userspace/runtime/c/include/stdlib.h"

namespace v8 {
namespace internal {

Heap::Heap(v8::PageAllocator* page_allocator)
    : page_allocator_(page_allocator)
    , undefined_value_(JSOldBall::UNDEFINED_KIND)
    , null_value_(JSOldBall::NULL_KIND)
    , true_value_(JSOldBall::TRUE_KIND)
    , false_value_(JSOldBall::FALSE_KIND)
    , allocated_bytes_(0)
    , gc_count_(0)
    , gc_threshold_(256 * 1024) // 256KB trigger threshold
{
}

Heap::~Heap() {
    for (auto* obj : young_space_) delete obj;
    for (auto* obj : old_space_) delete obj;
}

JSNumber* Heap::AllocateNumber(double val) {
    if (allocated_bytes_ >= gc_threshold_) {
        CollectGarbage();
    }
    JSNumber* num = new JSNumber(val);
    young_space_.push_back(num);
    allocated_bytes_ += sizeof(JSNumber);
    return num;
}

JSString* Heap::AllocateString(const std::string& str) {
    if (allocated_bytes_ >= gc_threshold_) {
        CollectGarbage();
    }
    JSString* s = new JSString(str);
    young_space_.push_back(s);
    allocated_bytes_ += sizeof(JSString) + str.size();
    return s;
}

JSObject* Heap::AllocateObject() {
    if (allocated_bytes_ >= gc_threshold_) {
        CollectGarbage();
    }
    JSObject* obj = new JSObject();
    young_space_.push_back(obj);
    allocated_bytes_ += sizeof(JSObject);
    return obj;
}

JSArray* Heap::AllocateArray(size_t len) {
    if (allocated_bytes_ >= gc_threshold_) {
        CollectGarbage();
    }
    JSArray* arr = new JSArray(len);
    young_space_.push_back(arr);
    allocated_bytes_ += sizeof(JSArray) + (len * sizeof(HeapObject*));
    return arr;
}

JSFunction* Heap::AllocateFunction(const std::string& name, BytecodeArray* bc) {
    if (allocated_bytes_ >= gc_threshold_) {
        CollectGarbage();
    }
    JSFunction* fn = new JSFunction(name, bc);
    young_space_.push_back(fn);
    allocated_bytes_ += sizeof(JSFunction);
    return fn;
}

JSFunction* Heap::AllocateNativeFunction(const std::string& name, NativeFunctionPtr callback) {
    if (allocated_bytes_ >= gc_threshold_) {
        CollectGarbage();
    }
    JSFunction* fn = new JSFunction(name, callback);
    young_space_.push_back(fn);
    allocated_bytes_ += sizeof(JSFunction);
    return fn;
}

void Heap::PushRoot(HeapObject** root_slot) {
    roots_.push_back(root_slot);
}

void Heap::PopRoot() {
    if (!roots_.empty()) {
        roots_.pop_back();
    }
}

void Heap::MarkObject(HeapObject* obj) {
    if (!obj || obj->is_marked) return;
    obj->is_marked = true;

    if (obj->type == JS_OBJECT_TYPE || obj->type == JS_FUNCTION_TYPE) {
        JSObject* jobj = static_cast<JSObject*>(obj);
        for (const auto& prop : jobj->properties) {
            MarkObject(prop.value);
        }
    } else if (obj->type == JS_ARRAY_TYPE) {
        JSArray* jarr = static_cast<JSArray*>(obj);
        for (auto* elem : jarr->elements) {
            MarkObject(elem);
        }
    }
}

void Heap::Scavenge() {
    gc_count_++;

    // 1. Mark live objects reachable from root set
    for (auto** slot : roots_) {
        if (slot && *slot) {
            MarkObject(*slot);
        }
    }

    // 2. Separate survivors into to-space / promote to old-space
    std::vector<HeapObject*> survivors;
    for (auto* obj : young_space_) {
        if (obj->is_marked) {
            obj->is_marked = false;
            survivors.push_back(obj);
        } else {
            delete obj;
        }
    }

    young_space_ = survivors;
    allocated_bytes_ = young_space_.size() * sizeof(JSObject);
}

void Heap::MarkSweep() {
    Scavenge();
}

void Heap::CollectGarbage(bool force_full_gc) {
    (void)force_full_gc;
    Scavenge();
}

} // namespace internal
} // namespace v8
