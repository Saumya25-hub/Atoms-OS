/*
 * Abseil Primitives Probe for ATOMS OS
 * Adapted from Google Abseil C++ (Apache 2.0)
 * Copyright © 2026 The Abseil Authors / ATOMS OS Adaptation
 */

#ifndef ABSL_PROBE_H
#define ABSL_PROBE_H

#include "userspace/runtime/cpp/include/string"
#include "userspace/runtime/c/include/string.h"
#include <stddef.h>
#include <stdint.h>

namespace absl {

// 1. absl::string_view
class string_view {
private:
    const char *ptr_;
    size_t length_;

public:
    constexpr string_view() : ptr_(""), length_(0) {}
    constexpr string_view(const char *str, size_t len) : ptr_(str), length_(len) {}
    string_view(const char *str) : ptr_(str), length_(str ? strlen(str) : 0) {}
    string_view(const std::string &s) : ptr_(s.data()), length_(s.size()) {}

    constexpr const char *data() const { return ptr_; }
    constexpr size_t size() const { return length_; }
    constexpr size_t length() const { return length_; }
    constexpr bool empty() const { return length_ == 0; }

    constexpr char operator[](size_t idx) const { return ptr_[idx]; }

    bool operator==(string_view other) const {
        return length_ == other.length_ && memcmp(ptr_, other.ptr_, length_) == 0;
    }
    bool operator!=(string_view other) const { return !(*this == other); }
};

// 2. absl::StatusCode
enum class StatusCode : int {
    kOk = 0,
    kCancelled = 1,
    kInvalidArgument = 3,
    kNotFound = 5,
    kInternal = 13,
    kUnavailable = 14,
};

// 3. absl::Status
class Status {
private:
    StatusCode code_;
    std::string message_;

public:
    Status() : code_(StatusCode::kOk), message_("") {}
    Status(StatusCode code, string_view msg) : code_(code), message_(msg.data()) {}

    bool ok() const { return code_ == StatusCode::kOk; }
    StatusCode code() const { return code_; }
    string_view message() const { return string_view(message_); }

    static Status OkStatus() { return Status(); }
};

bool RunAbseilProbe(void);

} // namespace absl

#endif /* ABSL_PROBE_H */
