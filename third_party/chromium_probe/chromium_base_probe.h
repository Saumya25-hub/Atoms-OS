/*
 * Chromium Base Primitives Probe for ATOMS OS
 * Adapted from Chromium base/ (BSD 3-Clause)
 * Copyright © 2026 The Chromium Authors / ATOMS OS Adaptation
 */

#ifndef CHROMIUM_BASE_PROBE_H
#define CHROMIUM_BASE_PROBE_H

#include "userspace/runtime/cpp/include/atomic"
#include "userspace/runtime/cpp/include/chrono"
#include "userspace/runtime/cpp/include/string"
#include <stddef.h>
#include <stdint.h>

namespace base {

// 1. base::TimeTicks
class TimeTicks {
private:
    int64_t us_;

public:
    constexpr TimeTicks() : us_(0) {}
    constexpr explicit TimeTicks(int64_t us) : us_(us) {}

    static TimeTicks Now() {
        auto now = std::chrono::high_resolution_clock::now();
        return TimeTicks(now.time_since_epoch().count() / 1000);
    }

    bool is_null() const { return us_ == 0; }
    int64_t InMicroseconds() const { return us_; }
    int64_t InMilliseconds() const { return us_ / 1000; }
};

// 2. base::span
template <typename T>
class span {
private:
    T *data_;
    size_t size_;

public:
    constexpr span() : data_(nullptr), size_(0) {}
    constexpr span(T *data, size_t size) : data_(data), size_(size) {}

    T *data() const { return data_; }
    size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }

    T &operator[](size_t idx) const { return data_[idx]; }
};

// 3. base::AtomicRefCount
class AtomicRefCount {
private:
    std::atomic<int> ref_count_;

public:
    constexpr AtomicRefCount() : ref_count_(0) {}
    constexpr explicit AtomicRefCount(int initial_value) : ref_count_(initial_value) {}

    void Increment() { ref_count_.fetch_add(1); }
    bool Decrement() { return ref_count_.fetch_sub(1) == 1; }
    bool IsOne() const { return ref_count_.load() == 1; }
    bool IsZero() const { return ref_count_.load() == 0; }
};

bool RunChromiumBaseProbe(void);

} // namespace base

#endif /* CHROMIUM_BASE_PROBE_H */
