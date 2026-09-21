/* Copyright (c) 2008-2015, Avian Contributors
   Portions Copyright (c) 2026, ATOMS OS Project / Saumya Chaudhari

   Permission to use, copy, modify, and/or distribute this software
   for any purpose with or without fee is hereby granted, provided
   that the above copyright notice and this permission notice appear
   in all copies.

   There is NO WARRANTY for this software. See LICENSE.txt for details. */

#ifndef AVIAN_SYSTEM_MEMORY_H
#define AVIAN_SYSTEM_MEMORY_H

#include <avian/util/slice.h>
#include <stdint.h>
#include <stddef.h>

namespace avian {
namespace system {

class Memory {
 public:
  enum Permissions {
    Read    = 1 << 0,
    Write   = 1 << 1,
    Execute = 1 << 2
  };
};

} // namespace system
} // namespace avian

#endif // AVIAN_SYSTEM_MEMORY_H
