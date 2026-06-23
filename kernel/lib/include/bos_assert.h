#pragma once

// Declare the panic function implemented in exception.c
void kernel_panic_assert(const char* file, int line, const char* func);

// BOS ASSERT macro
#define BOS_ASSERT(cond) \
    if (!(cond)) { \
        kernel_panic_assert(__FILE__, __LINE__, __func__); \
    }
