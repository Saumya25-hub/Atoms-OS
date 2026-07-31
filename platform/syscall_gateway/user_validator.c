#include "platform/include/bos_types.h"

/* User Range Memory Validation Contract */
bool BOS_ValidateUserPointer(const void* ptr, size_t size, bool check_writable) {
    if (!ptr) return false;
    uint64_t addr = (uint64_t)ptr;

    /* Reject NULL page and kernel upper memory region pointers (User space: 0x01000000 -> 0x7FFFFFFFFFFF) */
    if (addr < 0x0000000001000000ULL || addr >= 0x00007FFFFFFFFFFFULL) {
        return false;
    }

    if (addr + size < addr) return false; /* Overflow check */
    if (addr + size > 0x00007FFFFFFFFFFFULL) return false;

    (void)check_writable;
    return true;
}

bool BOS_ValidateUserString(const char* str, size_t max_len) {
    if (!str) return false;
    if (!BOS_ValidateUserPointer(str, 1, false)) return false;

    for (size_t i = 0; i < max_len; i++) {
        if (str[i] == '\0') return true;
    }
    return false;
}
