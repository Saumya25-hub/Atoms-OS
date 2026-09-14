/*
 * ATOMS Platform Adaptation Layer (APAL)
 * Shared Memory Implementation
 * Backed by ATOMS OS in-kernel Shared Physical Memory Manager (SYS_SHM_CALL)
 */

#include "apal_shm.h"
#include "atoms/userspace/runtime/include/atoms_syscall.h"
#include <string.h>

static volatile uint32_t g_shm_seq = 1;

apal_status_t apal_shm_create(size_t size, bool readonly, apal_shm_handle_t *out_handle) {
    if (size == 0 || !out_handle) return APAL_ERR_INVALID_PARAM;

    char shm_name[32];
    uint32_t seq = __atomic_fetch_add(&g_shm_seq, 1, __ATOMIC_SEQ_CST);
    shm_name[0] = 's'; shm_name[1] = 'h'; shm_name[2] = 'm'; shm_name[3] = '_';
    int p = 4;
    uint32_t tmp = seq;
    char rev[12]; int r = 0;
    if (tmp == 0) rev[r++] = '0';
    while (tmp > 0) { rev[r++] = '0' + (tmp % 10); tmp /= 10; }
    while (r > 0) shm_name[p++] = rev[--r];
    shm_name[p] = '\0';

    return apal_shm_create_named(shm_name, size, readonly, out_handle);
}

apal_status_t apal_shm_create_named(const char *name, size_t size, bool readonly, apal_shm_handle_t *out_handle) {
    if (!name || size == 0 || !out_handle) return APAL_ERR_INVALID_PARAM;

    uint32_t flags = readonly ? 1U : 3U; /* 1=Read, 3=Read/Write */
    int64_t handle = atoms_sys_shm_call(ATOMS_SHM_OP_CREATE, (uint64_t)name, (uint64_t)size, (uint64_t)flags);
    if (handle > 0) {
        *out_handle = (apal_shm_handle_t)handle;
        return APAL_OK;
    }
    return APAL_ERR_NO_MEMORY;
}

apal_status_t apal_shm_open_named(const char *name, bool readonly, apal_shm_handle_t *out_handle) {
    if (!name || !out_handle) return APAL_ERR_INVALID_PARAM;

    uint32_t flags = readonly ? 1U : 3U;
    int64_t handle = atoms_sys_shm_call(ATOMS_SHM_OP_OPEN, (uint64_t)name, (uint64_t)flags, 0);
    if (handle > 0) {
        *out_handle = (apal_shm_handle_t)handle;
        return APAL_OK;
    }
    return APAL_ERR_NOT_FOUND;
}

apal_status_t apal_shm_map(apal_shm_handle_t handle, size_t size, bool readonly, apal_shm_mapping_t *out_mapping) {
    if (handle == 0 || !out_mapping) return APAL_ERR_INVALID_PARAM;

    uint32_t perm = readonly ? 1U : 3U;
    int64_t virt_addr = atoms_sys_shm_call(ATOMS_SHM_OP_MAP, (uint64_t)handle, (uint64_t)perm, 0);
    if (virt_addr > 0) {
        out_mapping->handle = handle;
        out_mapping->mapped_addr = (void *)virt_addr;
        out_mapping->size = size;
        out_mapping->is_readonly = readonly;
        return APAL_OK;
    }
    return APAL_ERR_NO_MEMORY;
}

apal_status_t apal_shm_unmap(apal_shm_mapping_t *mapping) {
    if (!mapping || mapping->handle == 0 || !mapping->mapped_addr) return APAL_ERR_INVALID_PARAM;

    int64_t res = atoms_sys_shm_call(ATOMS_SHM_OP_UNMAP, (uint64_t)mapping->handle, (uint64_t)mapping->mapped_addr, 0);
    mapping->mapped_addr = NULL;
    mapping->size = 0;
    return (res == 0) ? APAL_OK : APAL_ERR_INVALID_PARAM;
}

apal_status_t apal_shm_close(apal_shm_handle_t handle) {
    if (handle == 0) return APAL_ERR_INVALID_PARAM;

    int64_t res = atoms_sys_shm_call(ATOMS_SHM_OP_DESTROY, (uint64_t)handle, 0, 0);
    return (res == 0) ? APAL_OK : APAL_ERR_INVALID_PARAM;
}
