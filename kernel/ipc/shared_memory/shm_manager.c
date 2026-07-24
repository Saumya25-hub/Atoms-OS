/*
 * BOS OS — Phase 2: IPC & Shared Memory Engine
 * shm_manager.c — Shared Memory Object Lifecycle Manager Implementation
 *
 * Allocates physical pages via pmm_alloc_page() and maps them into
 * process address spaces via vmm_map_page(). Supports page aliasing:
 * two virtual addresses pointing to the same physical page.
 */

#include "kernel/ipc/shared_memory/shm_manager.h"
#include "kernel/ipc/permissions/ipc_permissions.h"
#include "kernel/ipc/debug/ipc_debug.h"
#include "kernel/core/memory/pmm/include/pmm.h"
#include "kernel/core/memory/vmm/include/vmm.h"

static ipc_shm_object_t g_shm_objects[IPC_MAX_SHM_OBJECTS];
static uint32_t          g_next_shm_id = 1;

/* SHM virtual address base for kernel-side mappings */
#define IPC_SHM_VIRT_BASE    0xFFFF900000000000ULL
#define IPC_SHM_VIRT_STRIDE  (IPC_MAX_SHM_PAGES * PAGE_SIZE)

static bool shm_str_equal(const char* a, const char* b) {
    if (!a || !b) return false;
    while (*a && *b) {
        if (*a != *b) return false;
        a++; b++;
    }
    return (*a == *b);
}

static void shm_str_copy(char* dst, const char* src, uint32_t max_len) {
    uint32_t i = 0;
    while (src[i] && i < max_len) { dst[i] = src[i]; i++; }
    dst[i] = '\0';
}

void ipc_shm_manager_init(void) {
    for (uint32_t i = 0; i < IPC_MAX_SHM_OBJECTS; i++) {
        g_shm_objects[i].id         = 0;
        g_shm_objects[i].state      = IPC_SHM_STATE_FREE;
        g_shm_objects[i].ref_count  = 0;
        g_shm_objects[i].page_count = 0;
        g_shm_objects[i].mapping_count = 0;
        g_shm_objects[i].name[0]    = '\0';
    }
    g_next_shm_id = 1;
    ipc_debug_log(IPC_LOG_INFO, "SHM_MGR", "Shared Memory Manager Initialized");
}

ipc_status_t ipc_shm_create(const char* name, uint32_t size_bytes,
                              uint32_t flags, uint32_t owner_pid,
                              ipc_shm_handle_t* out_handle) {
    if (!out_handle) return IPC_ERR_NULL_POINTER;
    *out_handle = IPC_INVALID_HANDLE;

    if (size_bytes == 0) return IPC_ERR_INVALID_SIZE;

    /* Calculate required pages */
    uint32_t page_count = (size_bytes + PAGE_SIZE - 1) / PAGE_SIZE;
    if (page_count > IPC_MAX_SHM_PAGES) return IPC_ERR_INVALID_SIZE;

    /* Check for duplicate name */
    if (name) {
        for (uint32_t i = 0; i < IPC_MAX_SHM_OBJECTS; i++) {
            if (g_shm_objects[i].state == IPC_SHM_STATE_CREATED &&
                shm_str_equal(g_shm_objects[i].name, name)) {
                return IPC_ERR_NAME_EXISTS;
            }
        }
    }

    /* Find free slot */
    for (uint32_t i = 0; i < IPC_MAX_SHM_OBJECTS; i++) {
        if (g_shm_objects[i].state == IPC_SHM_STATE_FREE) {
            ipc_shm_object_t* obj = &g_shm_objects[i];

            /* Allocate physical pages */
            for (uint32_t p = 0; p < page_count; p++) {
                void* page = pmm_alloc_page();
                if (!page) {
                    /* Rollback already allocated pages */
                    for (uint32_t r = 0; r < p; r++) {
                        pmm_free_page((void*)obj->phys_pages[r]);
                    }
                    ipc_debug_log(IPC_LOG_ERROR, "SHM_MGR", "Physical page allocation failed");
                    return IPC_ERR_NO_MEMORY;
                }
                obj->phys_pages[p] = (uint64_t)page;

                /* Zero the page via identity-mapped address */
                uint8_t* ptr = (uint8_t*)page;
                for (uint32_t z = 0; z < PAGE_SIZE; z++) ptr[z] = 0;
            }

            obj->id         = g_next_shm_id++;
            obj->state      = IPC_SHM_STATE_CREATED;
            obj->owner_pid  = owner_pid;
            obj->size_bytes = size_bytes;
            obj->page_count = page_count;
            obj->mapping_count = 0;
            obj->ref_count  = 1;
            obj->flags      = flags;

            if (name) {
                shm_str_copy(obj->name, name, IPC_MAX_NAME_LENGTH);
            } else {
                obj->name[0] = '\0';
            }

            for (uint32_t m = 0; m < IPC_MAX_SHM_MAPPINGS; m++) {
                obj->mappings[m].active = false;
            }

            *out_handle = i;
            return IPC_SUCCESS;
        }
    }

    return IPC_ERR_MAX_SHM_OBJECTS;
}

ipc_status_t ipc_shm_open_by_name(const char* name, uint32_t flags,
                                   ipc_shm_handle_t* out_handle) {
    if (!name || !out_handle) return IPC_ERR_NULL_POINTER;
    *out_handle = IPC_INVALID_HANDLE;
    (void)flags;

    for (uint32_t i = 0; i < IPC_MAX_SHM_OBJECTS; i++) {
        if (g_shm_objects[i].state == IPC_SHM_STATE_CREATED &&
            shm_str_equal(g_shm_objects[i].name, name)) {
            g_shm_objects[i].ref_count++;
            *out_handle = i;
            return IPC_SUCCESS;
        }
    }

    return IPC_ERR_NAME_NOT_FOUND;
}

ipc_status_t ipc_shm_map_to_process(ipc_shm_handle_t handle,
                                     uint32_t pid, uint32_t perm,
                                     void** out_addr) {
    ipc_status_t vs = ipc_perm_validate_shm_handle(handle);
    if (vs != IPC_SUCCESS) return vs;
    if (!out_addr) return IPC_ERR_NULL_POINTER;
    *out_addr = (void*)0;

    ipc_shm_object_t* obj = &g_shm_objects[handle];
    if (obj->state != IPC_SHM_STATE_CREATED) return IPC_ERR_INVALID_HANDLE;

    /* Check for duplicate mapping */
    for (uint32_t m = 0; m < IPC_MAX_SHM_MAPPINGS; m++) {
        if (obj->mappings[m].active && obj->mappings[m].pid == pid) {
            return IPC_ERR_ALREADY_MAPPED;
        }
    }

    /* Find free mapping slot */
    uint32_t slot = IPC_MAX_SHM_MAPPINGS;
    for (uint32_t m = 0; m < IPC_MAX_SHM_MAPPINGS; m++) {
        if (!obj->mappings[m].active) {
            slot = m;
            break;
        }
    }
    if (slot >= IPC_MAX_SHM_MAPPINGS) return IPC_ERR_MAX_SHM_OBJECTS;

    /* Compute virtual address for this mapping */
    uint64_t virt_base = IPC_SHM_VIRT_BASE +
                          ((uint64_t)handle * IPC_MAX_SHM_MAPPINGS + slot) * IPC_SHM_VIRT_STRIDE;

    /* Get page table for target process */
    void* pml4 = vmm_get_active_pml4();

    /* Map each physical page into the virtual address range */
    uint32_t vmm_flags = 0x03; /* Present + Writable */
    if (!(perm & IPC_SHM_WRITE)) vmm_flags = 0x01; /* Present, Read-Only */

    for (uint32_t p = 0; p < obj->page_count; p++) {
        vmm_map_page(pml4, obj->phys_pages[p],
                     virt_base + (uint64_t)p * PAGE_SIZE, vmm_flags);
    }

    /* Record mapping */
    obj->mappings[slot].pid         = pid;
    obj->mappings[slot].virt_addr   = virt_base;
    obj->mappings[slot].permissions = perm;
    obj->mappings[slot].active      = true;
    obj->mapping_count++;

    *out_addr = (void*)virt_base;
    return IPC_SUCCESS;
}

ipc_status_t ipc_shm_unmap_from_process(ipc_shm_handle_t handle,
                                         uint32_t pid) {
    ipc_status_t vs = ipc_perm_validate_shm_handle(handle);
    if (vs != IPC_SUCCESS) return vs;

    ipc_shm_object_t* obj = &g_shm_objects[handle];
    if (obj->state != IPC_SHM_STATE_CREATED) return IPC_ERR_INVALID_HANDLE;

    for (uint32_t m = 0; m < IPC_MAX_SHM_MAPPINGS; m++) {
        if (obj->mappings[m].active && obj->mappings[m].pid == pid) {
            void* pml4 = vmm_get_active_pml4();
            for (uint32_t p = 0; p < obj->page_count; p++) {
                vmm_unmap_page(pml4,
                               obj->mappings[m].virt_addr + (uint64_t)p * PAGE_SIZE);
            }
            obj->mappings[m].active = false;
            obj->mapping_count--;
            return IPC_SUCCESS;
        }
    }

    return IPC_ERR_NOT_MAPPED;
}

ipc_status_t ipc_shm_close_handle(ipc_shm_handle_t handle) {
    ipc_status_t vs = ipc_perm_validate_shm_handle(handle);
    if (vs != IPC_SUCCESS) return vs;

    ipc_shm_object_t* obj = &g_shm_objects[handle];
    if (obj->state == IPC_SHM_STATE_FREE) return IPC_ERR_ALREADY_DESTROYED;

    if (obj->ref_count > 0) obj->ref_count--;
    return IPC_SUCCESS;
}

ipc_status_t ipc_shm_destroy_object(ipc_shm_handle_t handle) {
    ipc_status_t vs = ipc_perm_validate_shm_handle(handle);
    if (vs != IPC_SUCCESS) return vs;

    ipc_shm_object_t* obj = &g_shm_objects[handle];
    if (obj->state == IPC_SHM_STATE_FREE) return IPC_ERR_ALREADY_DESTROYED;
    if (obj->state == IPC_SHM_STATE_DESTROYED) return IPC_ERR_DOUBLE_DESTROY;

    /* Unmap all active mappings */
    for (uint32_t m = 0; m < IPC_MAX_SHM_MAPPINGS; m++) {
        if (obj->mappings[m].active) {
            void* pml4 = vmm_get_active_pml4();
            for (uint32_t p = 0; p < obj->page_count; p++) {
                vmm_unmap_page(pml4,
                               obj->mappings[m].virt_addr + (uint64_t)p * PAGE_SIZE);
            }
            obj->mappings[m].active = false;
        }
    }

    /* Free physical pages */
    for (uint32_t p = 0; p < obj->page_count; p++) {
        pmm_free_page((void*)obj->phys_pages[p]);
        obj->phys_pages[p] = 0;
    }

    obj->state      = IPC_SHM_STATE_FREE;
    obj->id         = 0;
    obj->page_count = 0;
    obj->mapping_count = 0;
    obj->ref_count  = 0;
    obj->name[0]    = '\0';

    ipc_debug_log(IPC_LOG_INFO, "SHM_MGR", "Shared Memory object destroyed & pages freed");
    return IPC_SUCCESS;
}

ipc_shm_object_t* ipc_shm_get(ipc_shm_handle_t handle) {
    if (handle >= IPC_MAX_SHM_OBJECTS) return (void*)0;
    if (g_shm_objects[handle].state == IPC_SHM_STATE_FREE) return (void*)0;
    return &g_shm_objects[handle];
}

bool ipc_shm_is_valid(ipc_shm_handle_t handle) {
    if (handle >= IPC_MAX_SHM_OBJECTS) return false;
    return (g_shm_objects[handle].state == IPC_SHM_STATE_CREATED);
}
