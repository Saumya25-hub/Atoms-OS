/*
 * ATOMS OS — GN/Ninja Toolchain C11 Test
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 */

#include "userspace/runtime/c/include/stdio.h"
#include "userspace/runtime/c/include/stdlib.h"
#include "userspace/runtime/c/include/string.h"
#include "userspace/runtime/c/include/unistd.h"

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "[TOOLCHAIN_C_TEST] PID=%d STATUS=PASS", (int)getpid());
    puts(buffer);

    void *ptr = malloc(512);
    if (!ptr) return 1;
    memset(ptr, 0x5A, 512);
    free(ptr);

    return 0;
}
