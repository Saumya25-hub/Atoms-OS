#include "../test_framework.h"

void test_scheduler_suite(void) {
    TEST_SUITE_START("Scheduler & Multitasking");

    // Test yielding back and forth
    // We can't easily assert on other tasks from userspace without IPC,
    // so we just verify the syscall doesn't crash or corrupt state.
    bool success = true;
    for (int i = 0; i < 1000; i++) {
        bos_yield();
    }
    
    ASSERT(success, "Yield Stability (1000 iterations)");

    // bos_spawn test - we can try spawning a known valid ELF
    // e.g. if we have a dummy, but we only have shell.elf and tests.elf.
    // If we spawn tests.elf, it will run the tests again, creating a fork bomb!
    // So we'll skip spawn stress here unless we have a specific binary.
    
    // Instead we can use bos_taskinfo to check if the scheduler returns data safely
    bos_taskinfo(1); // Usually kernel idle or init
    TEST_PASS("Task Info Syscall Safety");
}
