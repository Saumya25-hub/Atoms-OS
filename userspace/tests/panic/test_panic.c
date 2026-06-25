#include "../test_framework.h"

void test_panic_suite(void) {
    TEST_SUITE_START("Panic & Fault Recovery");

    bos_print("Spawning faulting process to test kernel task cleanup...\n");
    // Spawn fault.elf which will try to write to NULL pointer
    bos_spawn("fault.elf");
    
    // Yield a few times to ensure the kernel scheduler runs fault.elf
    // and terminates it, and then we continue running.
    for (int i = 0; i < 100; i++) {
        bos_yield();
    }
    
    // If we reach here, it means tests.elf wasn't killed and the kernel didn't panic!
    TEST_PASS("Userspace Fault Isolation");
}
