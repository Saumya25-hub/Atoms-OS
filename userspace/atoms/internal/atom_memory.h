#ifndef ATOM_MEMORY_H
#define ATOM_MEMORY_H

#include <stddef.h>
#include <stdint.h>

// Platform bridge for memory allocation.
// In V1 userspace, this will map to a simple internal allocator since bos_brk is not yet present.

void* atom_alloc(size_t size);
void  atom_free(void* ptr);

#endif // ATOM_MEMORY_H
