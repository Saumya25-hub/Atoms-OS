/* Copyright (c) 2008-2015, Avian Contributors
   Portions Copyright (c) 2026, ATOMS OS Project / Saumya Chaudhari

   Permission to use, copy, modify, and/or distribute this software
   for any purpose with or without fee is hereby granted, provided
   that the above copyright notice and this permission notice appear
   in all copies.

   There is NO WARRANTY for this software. See LICENSE.txt for details. */

#ifndef AVIAN_PROCESSOR_H
#define AVIAN_PROCESSOR_H

#include <avian/common.h>
#include <avian/system/system.h>
#include <avian/heap/heap.h>
#include <avian/classfile.h>
#include <avian/machine.h>

namespace avian {

class InterpreterProcessor {
 public:
  virtual ~InterpreterProcessor() {}
  virtual int32_t execute(const uint8_t* code, size_t length) = 0;
};

InterpreterProcessor* makeProcessor(system::System* s, heap::Heap* h);

} // namespace avian

#endif // AVIAN_PROCESSOR_H
