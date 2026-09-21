/* Copyright (c) 2008-2015, Avian Contributors
   Portions Copyright (c) 2026, ATOMS OS Project / Saumya Chaudhari

   Permission to use, copy, modify, and/or distribute this software
   for any purpose with or without fee is hereby granted, provided
   that the above copyright notice and this permission notice appear
   in all copies.

   There is NO WARRANTY for this software. See LICENSE.txt for details. */

#ifndef AVIAN_UTIL_SLICE_H
#define AVIAN_UTIL_SLICE_H

#include <stddef.h>
#include <stdint.h>

namespace avian {
namespace util {

template <class T>
class Slice {
 public:
  Slice() : data_(0), count_(0) {}
  Slice(T* data, size_t count) : data_(data), count_(count) {}

  T* begin() const { return data_; }
  T* end() const { return data_ + count_; }
  size_t count() const { return count_; }
  T& operator[](size_t index) const { return data_[index]; }

 private:
  T* data_;
  size_t count_;
};

} // namespace util
} // namespace avian

#endif // AVIAN_UTIL_SLICE_H
