/*
 * ATOMS OS — Userspace C++ Runtime Core Support
 * Adapted from LLVM libc++abi (Apache 2.0 with LLVM Exception)
 */

#include "../include/new"
#include "../include/typeinfo"
#include "../../c/include/stdlib.h"
#include "../../c/include/stdio.h"

namespace std {
    const nothrow_t nothrow{};
    type_info::~type_info() {}
}


void *operator new(size_t size) {
    if (size == 0) size = 1;
    void *p = malloc(size);
    return p;
}

void *operator new[](size_t size) {
    return ::operator new(size);
}

void operator delete(void *ptr) noexcept {
    if (ptr) free(ptr);
}

void operator delete[](void *ptr) noexcept {
    ::operator delete(ptr);
}

void operator delete(void *ptr, size_t) noexcept {
    ::operator delete(ptr);
}

void operator delete[](void *ptr, size_t) noexcept {
    ::operator delete(ptr);
}

void *operator new(size_t size, const std::nothrow_t &) noexcept {
    return malloc(size);
}

void *operator new[](size_t size, const std::nothrow_t &) noexcept {
    return malloc(size);
}

void operator delete(void *ptr, const std::nothrow_t &) noexcept {
    free(ptr);
}

void operator delete[](void *ptr, const std::nothrow_t &) noexcept {
    free(ptr);
}

extern "C" {

void __cxa_pure_virtual(void) {
    puts("[C++ RUNTIME FATAL] Pure virtual function called!");
    abort();
}

int __cxa_atexit(void (*func)(void *), void *arg, void *dso_handle) {
    (void)func; (void)arg; (void)dso_handle;
    return 0;
}

int __cxa_guard_acquire(uint64_t *guard) {
    return !*(uint8_t*)guard;
}

void __cxa_guard_release(uint64_t *guard) {
    *(uint8_t*)guard = 1;
}

void __cxa_guard_abort(uint64_t *guard) {
    (void)guard;
}

}

