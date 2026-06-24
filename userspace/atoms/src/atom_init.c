#include "../include/atoms.h"
#include "../internal/atom_memory.h"
#include "../../libbos/include/bos.h"

void atoms_init(void) {
    // Step 1: Verify Platform
    void* test_alloc = atom_alloc(16);
    if (!test_alloc) {
        bos_print("[ATOMS] PANIC: Platform allocator not ready\n");
        bos_exit();
        while(1) bos_yield();
    }
    atom_free(test_alloc);
    
    // Step 2: Initialize String Intern Pool
    atom_string_pool_init();
    
    // Step 3: Initialize Debug System (Phase 3)
    // atom_debug_init();
    
    bos_print("[BOOT] Atoms V1 Ready\n");
}

void atoms_shutdown(void) {
    // Step 1: Destroy String Pool
    atom_string_pool_destroy();
    
    bos_print("[SHUTDOWN] Atoms V1 Shutdown\n");
}
