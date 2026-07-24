#ifndef ABE_RESOURCE_H
#define ABE_RESOURCE_H

#include "../../../sdk/include/abe/abe.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ABE_RESOURCE_TYPE_UNKNOWN = 0,
    ABE_RESOURCE_TYPE_TAB_NODE = 1,
    ABE_RESOURCE_TYPE_WINDOW_NODE = 2,
    ABE_RESOURCE_TYPE_NAV_ENTRY = 3,
    ABE_RESOURCE_TYPE_URL_NODE = 4,
    ABE_RESOURCE_TYPE_IMAGE = 5,
    ABE_RESOURCE_TYPE_FONT = 6,
    ABE_RESOURCE_TYPE_CACHE = 7,
    ABE_RESOURCE_TYPE_NETWORK = 8
} ABE_ResourceType;

typedef struct {
    ABE_ResourceHandle handle;
    ABE_ResourceType type;
    void* data_ptr;
    size_t data_size;
    uint32_t ref_count;
    uint32_t generation;
    bool is_allocated;
} ABE_ResourceDescriptor;

#define ABE_MEMORY_POOL_SLOTS 256

typedef struct {
    size_t block_size;
    uint32_t total_blocks;
    uint32_t free_blocks;
    uint8_t* pool_buffer;
    bool* block_allocated;
    size_t total_bytes_allocated;
} ABE_MemoryPool;

typedef struct {
    ABE_MemoryPool pool_tab_nodes;
    ABE_MemoryPool pool_window_nodes;
    ABE_MemoryPool pool_nav_entries;
    ABE_MemoryPool pool_url_nodes;
    ABE_ResourceDescriptor descriptors[ABE_MEMORY_POOL_SLOTS];
    uint32_t next_handle_index;
    uint32_t current_generation;
    size_t total_resource_bytes;
} ABE_ResourceManager;

ABE_Error ABE_ResourceManager_Init(size_t max_pool_bytes);
ABE_Error ABE_ResourceManager_Shutdown(void);

// Memory Pool Operations
void*     ABE_MemPool_Alloc(ABE_MemoryPool* pool, size_t size);
void      ABE_MemPool_Free(ABE_MemoryPool* pool, void* ptr);

// Handle Management Operations
ABE_Error ABE_Resource_Allocate(ABE_ResourceType type, size_t size, void** out_ptr, ABE_ResourceHandle* out_handle);
ABE_Error ABE_Resource_Free(ABE_ResourceHandle handle);
void*     ABE_Resource_GetPointer(ABE_ResourceHandle handle);
ABE_Error ABE_Resource_AddRef(ABE_ResourceHandle handle);
ABE_Error ABE_Resource_ReleaseRef(ABE_ResourceHandle handle);

void      ABE_Resource_DumpAudit(void);

#ifdef __cplusplus
}
#endif

#endif // ABE_RESOURCE_H
