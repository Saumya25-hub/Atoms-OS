#include "../../libbos/include/bos.h"

void _start(void) {
    // Attempt to write to a NULL pointer to cause a page fault in userspace (Ring 3)
    volatile int* ptr = (volatile int*)0x00000000;
    *ptr = 0xDEADBEEF; // Should trigger exception 14 (Page Fault)
    
    // We should never reach here
    bos_exit();
}
