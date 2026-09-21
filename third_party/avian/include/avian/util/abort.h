/* Copyright (c) 2008-2015, Avian Contributors
   Portions Copyright (c) 2026, ATOMS OS Project / Saumya Chaudhari

   Permission to use, copy, modify, and/or distribute this software
   for any purpose with or without fee is hereby granted, provided
   that the above copyright notice and this permission notice appear
   in all copies.

   There is NO WARRANTY for this software. See LICENSE.txt for details. */

#ifndef AVIAN_UTIL_ABORT_H
#define AVIAN_UTIL_ABORT_H

namespace avian {
namespace util {

class Aborter {
 public:
  virtual ~Aborter() {}
  virtual void abort() = 0;
};

} // namespace util
} // namespace avian

#endif // AVIAN_UTIL_ABORT_H
