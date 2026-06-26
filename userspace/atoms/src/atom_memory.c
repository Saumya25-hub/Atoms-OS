#include "../internal/atom_memory.h"
#include "../../libbos/include/bos.h"

// Simple static bump allocator for V1 BOS userspace.
// Since BOS doesn't have sys_brk or malloc in userspace yet,
// we use a 1MB static pool for Atoms.

#define ATOM_POOL_SIZE (1024 * 1024)
static uint8_t atom_pool[ATOM_POOL_SIZE];
static size_t atom_pool_offset = 0;

void* atom_alloc(size_t size) {
    // Align size to 8 bytes
    size_t aligned_size = (size + 7) & ~7ULL;
    
    if (atom_pool_offset + aligned_size > ATOM_POOL_SIZE) {
        return NULL;
    }
    
    void* ptr = &atom_pool[atom_pool_offset];
    atom_pool_offset += aligned_size;
    
    // Zero memory
    uint8_t* p = (uint8_t*)ptr;
    for (size_t i = 0; i < aligned_size; i++) {
        p[i] = 0;
    }
    
    return ptr;
}

void atom_free(void* ptr) {
    if (ptr == NULL) {
        atom_pool_offset = 0;
        return;
    }
    // Bump allocator cannot free individual blocks.
    // In V1 userspace testing, this is acceptable until a real malloc is ported.
}
