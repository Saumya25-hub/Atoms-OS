#include "abe_resource.h"
#include "../diagnostics/abe_diagnostics.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"

static ABE_ResourceManager g_res_manager;
static bool g_res_initialized = false;

static ABE_Error InitPool(ABE_MemoryPool* pool, size_t block_size, uint32_t block_count) {
    if (!pool || block_size == 0 || block_count == 0) return ABE_ERR_INVALID_PARAM;
    memset(pool, 0, sizeof(ABE_MemoryPool));
    pool->block_size = block_size;
    pool->total_blocks = block_count;
    pool->free_blocks = block_count;

    size_t buffer_bytes = block_size * block_count;
    pool->pool_buffer = (uint8_t*)kmalloc(buffer_bytes);
    if (!pool->pool_buffer) return ABE_ERR_OUT_OF_MEMORY;
    memset(pool->pool_buffer, 0, buffer_bytes);

    size_t flag_bytes = sizeof(bool) * block_count;
    pool->block_allocated = (bool*)kmalloc(flag_bytes);
    if (!pool->block_allocated) {
        kfree(pool->pool_buffer);
        pool->pool_buffer = NULL;
        return ABE_ERR_OUT_OF_MEMORY;
    }
    memset(pool->block_allocated, 0, flag_bytes);
    ABE_Diag_RecordMemoryAlloc(buffer_bytes + flag_bytes);
    return ABE_SUCCESS;
}

static void FreePool(ABE_MemoryPool* pool) {
    if (!pool) return;
    if (pool->pool_buffer) {
        size_t buffer_bytes = pool->block_size * pool->total_blocks;
        kfree(pool->pool_buffer);
        pool->pool_buffer = NULL;
        ABE_Diag_RecordMemoryFree(buffer_bytes);
    }
    if (pool->block_allocated) {
        size_t flag_bytes = sizeof(bool) * pool->total_blocks;
        kfree(pool->block_allocated);
        pool->block_allocated = NULL;
        ABE_Diag_RecordMemoryFree(flag_bytes);
    }
    pool->total_blocks = 0;
    pool->free_blocks = 0;
}

ABE_Error ABE_ResourceManager_Init(size_t max_pool_bytes) {
    (void)max_pool_bytes;
    memset(&g_res_manager, 0, sizeof(ABE_ResourceManager));
    g_res_manager.current_generation = 1;

    ABE_Error err;
    err = InitPool(&g_res_manager.pool_tab_nodes, 512, 64);
    if (err != ABE_SUCCESS) return err;

    err = InitPool(&g_res_manager.pool_window_nodes, 1024, 32);
    if (err != ABE_SUCCESS) return err;

    err = InitPool(&g_res_manager.pool_nav_entries, 256, 128);
    if (err != ABE_SUCCESS) return err;

    err = InitPool(&g_res_manager.pool_url_nodes, 2048, 64);
    if (err != ABE_SUCCESS) return err;

    g_res_initialized = true;
    ABE_Log(ABE_LOG_INFO, "RES", "ABE Resource Manager initialized with fixed-block pools");
    return ABE_SUCCESS;
}

ABE_Error ABE_ResourceManager_Shutdown(void) {
    if (!g_res_initialized) return ABE_ERR_NOT_INITIALIZED;

    for (uint32_t i = 0; i < ABE_MEMORY_POOL_SLOTS; i++) {
        if (g_res_manager.descriptors[i].is_allocated && g_res_manager.descriptors[i].data_ptr) {
            kfree(g_res_manager.descriptors[i].data_ptr);
            ABE_Diag_RecordMemoryFree(g_res_manager.descriptors[i].data_size);
            g_res_manager.descriptors[i].is_allocated = false;
        }
    }

    FreePool(&g_res_manager.pool_tab_nodes);
    FreePool(&g_res_manager.pool_window_nodes);
    FreePool(&g_res_manager.pool_nav_entries);
    FreePool(&g_res_manager.pool_url_nodes);

    g_res_initialized = false;
    ABE_Log(ABE_LOG_INFO, "RES", "ABE Resource Manager shut down cleanly");
    return ABE_SUCCESS;
}

void* ABE_MemPool_Alloc(ABE_MemoryPool* pool, size_t size) {
    if (!pool || !pool->pool_buffer || pool->free_blocks == 0) return NULL;
    if (size > pool->block_size) return NULL;

    for (uint32_t i = 0; i < pool->total_blocks; i++) {
        if (!pool->block_allocated[i]) {
            pool->block_allocated[i] = true;
            pool->free_blocks--;
            void* ptr = pool->pool_buffer + (i * pool->block_size);
            memset(ptr, 0, pool->block_size);
            return ptr;
        }
    }
    return NULL;
}

void ABE_MemPool_Free(ABE_MemoryPool* pool, void* ptr) {
    if (!pool || !pool->pool_buffer || !ptr) return;
    uint8_t* p = (uint8_t*)ptr;
    if (p < pool->pool_buffer || p >= (pool->pool_buffer + (pool->total_blocks * pool->block_size))) {
        return;
    }
    size_t offset = p - pool->pool_buffer;
    uint32_t index = (uint32_t)(offset / pool->block_size);
    if (index < pool->total_blocks && pool->block_allocated[index]) {
        pool->block_allocated[index] = false;
        pool->free_blocks++;
    }
}

ABE_Error ABE_Resource_Allocate(ABE_ResourceType type, size_t size, void** out_ptr, ABE_ResourceHandle* out_handle) {
    if (!g_res_initialized || !out_ptr || !out_handle || size == 0) return ABE_ERR_INVALID_PARAM;

    uint32_t slot = ABE_INVALID_HANDLE;
    for (uint32_t i = 0; i < ABE_MEMORY_POOL_SLOTS; i++) {
        uint32_t idx = (g_res_manager.next_handle_index + i) % ABE_MEMORY_POOL_SLOTS;
        if (!g_res_manager.descriptors[idx].is_allocated) {
            slot = idx;
            g_res_manager.next_handle_index = (idx + 1) % ABE_MEMORY_POOL_SLOTS;
            break;
        }
    }

    if (slot == ABE_INVALID_HANDLE) return ABE_ERR_RESOURCE_EXHAUSTED;

    void* ptr = kmalloc(size);
    if (!ptr) return ABE_ERR_OUT_OF_MEMORY;
    memset(ptr, 0, size);
    ABE_Diag_RecordMemoryAlloc(size);

    uint32_t gen = g_res_manager.current_generation++;
    ABE_ResourceHandle handle = (gen << 16) | (slot & 0xFFFF);

    ABE_ResourceDescriptor* desc = &g_res_manager.descriptors[slot];
    desc->handle = handle;
    desc->type = type;
    desc->data_ptr = ptr;
    desc->data_size = size;
    desc->ref_count = 1;
    desc->generation = gen;
    desc->is_allocated = true;

    *out_ptr = ptr;
    *out_handle = handle;
    return ABE_SUCCESS;
}

ABE_Error ABE_Resource_Free(ABE_ResourceHandle handle) {
    if (!g_res_initialized || handle == ABE_INVALID_HANDLE) return ABE_ERR_INVALID_PARAM;

    uint32_t slot = handle & 0xFFFF;
    uint32_t gen = (handle >> 16) & 0xFFFF;

    if (slot >= ABE_MEMORY_POOL_SLOTS) return ABE_ERR_INVALID_PARAM;
    ABE_ResourceDescriptor* desc = &g_res_manager.descriptors[slot];

    if (!desc->is_allocated || desc->generation != gen) return ABE_ERR_INVALID_PARAM;

    if (desc->data_ptr) {
        kfree(desc->data_ptr);
        ABE_Diag_RecordMemoryFree(desc->data_size);
        desc->data_ptr = NULL;
    }
    desc->is_allocated = false;
    desc->ref_count = 0;
    return ABE_SUCCESS;
}

void* ABE_Resource_GetPointer(ABE_ResourceHandle handle) {
    if (!g_res_initialized || handle == ABE_INVALID_HANDLE) return NULL;
    uint32_t slot = handle & 0xFFFF;
    uint32_t gen = (handle >> 16) & 0xFFFF;
    if (slot >= ABE_MEMORY_POOL_SLOTS) return NULL;
    ABE_ResourceDescriptor* desc = &g_res_manager.descriptors[slot];
    if (!desc->is_allocated || desc->generation != gen) return NULL;
    return desc->data_ptr;
}

ABE_Error ABE_Resource_AddRef(ABE_ResourceHandle handle) {
    if (!g_res_initialized || handle == ABE_INVALID_HANDLE) return ABE_ERR_INVALID_PARAM;
    uint32_t slot = handle & 0xFFFF;
    uint32_t gen = (handle >> 16) & 0xFFFF;
    if (slot >= ABE_MEMORY_POOL_SLOTS) return ABE_ERR_INVALID_PARAM;
    ABE_ResourceDescriptor* desc = &g_res_manager.descriptors[slot];
    if (!desc->is_allocated || desc->generation != gen) return ABE_ERR_INVALID_PARAM;
    desc->ref_count++;
    return ABE_SUCCESS;
}

ABE_Error ABE_Resource_ReleaseRef(ABE_ResourceHandle handle) {
    if (!g_res_initialized || handle == ABE_INVALID_HANDLE) return ABE_ERR_INVALID_PARAM;
    uint32_t slot = handle & 0xFFFF;
    uint32_t gen = (handle >> 16) & 0xFFFF;
    if (slot >= ABE_MEMORY_POOL_SLOTS) return ABE_ERR_INVALID_PARAM;
    ABE_ResourceDescriptor* desc = &g_res_manager.descriptors[slot];
    if (!desc->is_allocated || desc->generation != gen) return ABE_ERR_INVALID_PARAM;

    if (desc->ref_count > 0) desc->ref_count--;
    if (desc->ref_count == 0) {
        return ABE_Resource_Free(handle);
    }
    return ABE_SUCCESS;
}

void ABE_Resource_DumpAudit(void) {
    ABE_Log(ABE_LOG_INFO, "RES", "--- ABE Resource Manager Pool Audit ---");
    ABE_LogVal(ABE_LOG_INFO, "RES", "Tab Nodes Free: ", g_res_manager.pool_tab_nodes.free_blocks);
    ABE_LogVal(ABE_LOG_INFO, "RES", "Window Nodes Free: ", g_res_manager.pool_window_nodes.free_blocks);
    ABE_LogVal(ABE_LOG_INFO, "RES", "Nav Entries Free: ", g_res_manager.pool_nav_entries.free_blocks);
    ABE_LogVal(ABE_LOG_INFO, "RES", "URL Nodes Free: ", g_res_manager.pool_url_nodes.free_blocks);
}
