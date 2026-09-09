/*
 * ATOMS OS — Userspace C++ Runtime Adapter Glue
 * Provides minimal platform hooks for LLVM libc++ / libc++abi:
 * - std::get_new_handler / std::set_new_handler
 * - std::terminate handler
 */

#include <new>
#include <exception>
#include <stdlib.h>

namespace std {

static new_handler g_new_handler = nullptr;
static terminate_handler g_terminate_handler = nullptr;

new_handler get_new_handler() noexcept {
    return g_new_handler;
}

new_handler set_new_handler(new_handler nh) noexcept {
    new_handler old = g_new_handler;
    g_new_handler = nh;
    return old;
}

terminate_handler get_terminate() noexcept {
    return g_terminate_handler;
}

terminate_handler set_terminate(terminate_handler th) noexcept {
    terminate_handler old = g_terminate_handler;
    g_terminate_handler = th;
    return old;
}

void terminate() noexcept {
    if (g_terminate_handler) {
        g_terminate_handler();
    }
    abort();
}

} // namespace std
