/*
 * ATOMS OS — Generated Code Test Executable
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 */

#include "userspace/runtime/c/include/stdio.h"

extern void atoms_print_generated_info(void);

int main(void) {
    puts("[TOOLCHAIN_GENERATED_TEST] Testing Host-Generated Source Code...");
    atoms_print_generated_info();
    puts("[TOOLCHAIN_GENERATED_TEST] STATUS=PASS");
    return 0;
}
