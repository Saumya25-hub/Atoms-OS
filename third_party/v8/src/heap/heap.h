/*
 * Copyright 2014 the V8 project authors. All rights reserved.
 * Copyright 2026 The V8 Authors / ATOMS OS Adaptation
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef V8_HEAP_HEAP_H_
#define V8_HEAP_HEAP_H_

#include "third_party/v8/src/objects/objects.h"
#include "third_party/v8/src/base/page-allocator.h"
#include "userspace/runtime/cpp/include/vector"

namespace v8 {
namespace internal {

class Heap {
public:
    Heap(v8::PageAllocator* page_allocator);
    ~Heap();

    // Allocation methods
    JSNumber* AllocateNumber(double val);
    JSString* AllocateString(const std::string& str);
    JSObject* AllocateObject();
    JSArray* AllocateArray(size_t len = 0);
    JSFunction* AllocateFunction(const std::string& name, BytecodeArray* bc = nullptr);
    JSFunction* AllocateNativeFunction(const std::string& name, NativeFunctionPtr callback);

    JSOldBall* GetUndefined() { return &undefined_value_; }
    JSOldBall* GetNull() { return &null_value_; }
    JSOldBall* GetTrue() { return &true_value_; }
    JSOldBall* GetFalse() { return &false_value_; }

    // Root management for HandleScope
    void PushRoot(HeapObject** root_slot);
    void PopRoot();
    size_t GetRootCount() const { return roots_.size(); }

    // Garbage Collection
    void CollectGarbage(bool force_full_gc = false);
    size_t GetAllocatedBytes() const { return allocated_bytes_; }
    size_t GetGCCount() const { return gc_count_; }

private:
    v8::PageAllocator* page_allocator_;
    std::vector<HeapObject*> young_space_;
    std::vector<HeapObject*> old_space_;
    std::vector<HeapObject**> roots_;

    JSOldBall undefined_value_;
    JSOldBall null_value_;
    JSOldBall true_value_;
    JSOldBall false_value_;

    size_t allocated_bytes_;
    size_t gc_count_;
    size_t gc_threshold_;

    void Scavenge();
    void MarkSweep();
    void MarkObject(HeapObject* obj);
};

} // namespace internal
} // namespace v8

#endif // V8_HEAP_HEAP_H_
