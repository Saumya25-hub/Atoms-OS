/* Copyright (c) 2008-2015, Avian Contributors
   Portions Copyright (c) 2026, ATOMS OS Project / Saumya Chaudhari

   Permission to use, copy, modify, and/or distribute this software
   for any purpose with or without fee is hereby granted, provided
   that the above copyright notice and this permission notice appear
   in all copies.

   There is NO WARRANTY for this software. See LICENSE.txt for details. */

#ifndef AVIAN_COMMON_H
#define AVIAN_COMMON_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define AVIAN_VERSION "1.2.0"
#define AVIAN_ARCH_X86_64 1

#ifndef TARGET_BYTES_PER_WORD
#define TARGET_BYTES_PER_WORD 8
#endif

#define UNUSED __attribute__((unused))

namespace avian {

typedef intptr_t intptr;
typedef uintptr_t uintptr;

typedef int8_t  int8;
typedef uint8_t uint8;

typedef int16_t  int16;
typedef uint16_t uint16;

typedef int32_t  int32;
typedef uint32_t uint32;

typedef int64_t  int64;
typedef uint64_t uint64;

} // namespace avian

namespace vm {
using namespace avian;
}

#endif // AVIAN_COMMON_H
