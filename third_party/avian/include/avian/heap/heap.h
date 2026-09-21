/* Copyright (c) 2008-2015, Avian Contributors
   Portions Copyright (c) 2026, ATOMS OS Project / Saumya Chaudhari

   Permission to use, copy, modify, and/or distribute this software
   for any purpose with or without fee is hereby granted, provided
   that the above copyright notice and this permission notice appear
   in all copies.

   There is NO WARRANTY for this software. See LICENSE.txt for details. */

#ifndef AVIAN_HEAP_HEAP_H
#define AVIAN_HEAP_HEAP_H

#include <avian/common.h>
#include <avian/system/system.h>
#include <avian/util/allocator.h>

namespace avian {
namespace heap {

struct ObjectHeader {
    uint32_t flags;
    uint32_t size;
    void*    class_ptr;
};

class Heap {
 public:
  virtual ~Heap() {}

  virtual void* allocate(size_t size) = 0;
  virtual void* allocateObject(void* class_ptr, size_t payload_size) = 0;
  virtual void* allocateArray(void* element_class, size_t length, size_t element_size) = 0;
  virtual void collectGarbage() = 0;

  virtual size_t totalBytes() const = 0;
  virtual size_t usedBytes() const = 0;
  virtual size_t freeBytes() const = 0;
  virtual size_t allocationCount() const = 0;
};

Heap* makeHeap(system::System* s, size_t initialCapacity, size_t maximumCapacity);

} // namespace heap
} // namespace avian

namespace vm {
using namespace avian::heap;
}

#endif // AVIAN_HEAP_HEAP_H
