/*
 * ATOMS Platform Adaptation Layer (APAL)
 * Memory Subsystem Adapter (V8 & PartitionAlloc Bridge)
 */

#ifndef ATOMS_APAL_MEMORY_H
#define ATOMS_APAL_MEMORY_H

#include "../include/apal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

size_t apal_page_size(void);
size_t apal_allocation_granularity(void);

void *apal_page_alloc(void *hint, size_t size, apal_prot_t prot, uint32_t flags);
apal_status_t apal_page_free(void *addr, size_t size);
apal_status_t apal_page_protect(void *addr, size_t size, apal_prot_t prot);
apal_status_t apal_page_commit(void *addr, size_t size, apal_prot_t prot);
apal_status_t apal_page_decommit(void *addr, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_APAL_MEMORY_H */
