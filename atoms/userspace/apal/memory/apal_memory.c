/*
 * ATOMS Platform Adaptation Layer (APAL)
 * Memory Subsystem Implementation
 */

#include "apal_memory.h"
#include "atoms/userspace/runtime/include/atoms_syscall.h"

#define ATOMS_PAGE_SIZE 4096ULL
#define ATOMS_ALLOC_GRANULARITY 4096ULL

size_t apal_page_size(void) {
    return (size_t)ATOMS_PAGE_SIZE;
}

size_t apal_allocation_granularity(void) {
    return (size_t)ATOMS_ALLOC_GRANULARITY;
}

static int convert_prot(apal_prot_t prot) {
    int p = 0;
    if (prot & APAL_PROT_READ) p |= PROT_READ;
    if (prot & APAL_PROT_WRITE) p |= PROT_WRITE;
    if (prot & APAL_PROT_EXEC) p |= PROT_EXEC;
    return p;
}

void *apal_page_alloc(void *hint, size_t size, apal_prot_t prot, uint32_t flags) {
    if (size == 0) return NULL;
    
    /* Round up to page boundary */
    size_t aligned_size = (size + (ATOMS_PAGE_SIZE - 1)) & ~(ATOMS_PAGE_SIZE - 1);
    int native_prot = convert_prot(prot);
    int native_flags = MAP_ANONYMOUS | MAP_PRIVATE;
    
    if (hint != NULL) {
        native_flags |= MAP_FIXED;
    }

    void *ptr = atoms_sys_mmap(hint, aligned_size, native_prot, native_flags, -1, 0);
    if (ptr == (void *)-1 || ptr == NULL) {
        return NULL;
    }
    return ptr;
}

apal_status_t apal_page_free(void *addr, size_t size) {
    if (!addr || size == 0) return APAL_ERR_INVALID_PARAM;
    size_t aligned_size = (size + (ATOMS_PAGE_SIZE - 1)) & ~(ATOMS_PAGE_SIZE - 1);
    int res = atoms_sys_munmap(addr, aligned_size);
    return (res == 0) ? APAL_OK : APAL_ERR_INTERNAL;
}

apal_status_t apal_page_protect(void *addr, size_t size, apal_prot_t prot) {
    if (!addr || size == 0) return APAL_ERR_INVALID_PARAM;
    size_t aligned_size = (size + (ATOMS_PAGE_SIZE - 1)) & ~(ATOMS_PAGE_SIZE - 1);
    int native_prot = convert_prot(prot);
    int res = atoms_sys_mprotect(addr, aligned_size, native_prot);
    return (res == 0) ? APAL_OK : APAL_ERR_INTERNAL;
}

apal_status_t apal_page_commit(void *addr, size_t size, apal_prot_t prot) {
    /* In ATOMS anonymous mmap, pages are committed upon mprotect / access */
    return apal_page_protect(addr, size, prot);
}

apal_status_t apal_page_decommit(void *addr, size_t size) {
    /* Decommit changes protection to PROT_NONE to free physical frame backing */
    return apal_page_protect(addr, size, APAL_PROT_NONE);
}
