#ifndef _BOS_BVMM_TYPES_H_
#define _BOS_BVMM_TYPES_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file bvmm_types.h
 * @brief BOS VRAM Memory Manager (BVMM) Core Data Structures & Types
 * 
 * Production-grade data structure layout for memory domains, handles,
 * allocations, heaps, memory pools, and telemetry statistics.
 */

/* ========================================================================== */
/* Result Codes                                                               */
/* ========================================================================== */

typedef enum {
    BVMM_SUCCESS                 =  0,
    BVMM_ERR_INVALID_ARGUMENT    = -1,
    BVMM_ERR_OUT_OF_MEMORY       = -2,
    BVMM_ERR_INVALID_HANDLE      = -3,
    BVMM_ERR_NOT_INITIALIZED     = -4,
    BVMM_ERR_ALREADY_INITIALIZED = -5,
    BVMM_ERR_HANDLE_EXPIRED      = -6,
    BVMM_ERR_PERMISSION_DENIED   = -7,
    BVMM_ERR_BUSY                = -8,
    BVMM_ERR_NOT_SUPPORTED       = -9,
    BVMM_ERR_ALIGNMENT_INVALID   = -10
} bvmm_result_t;

/* ========================================================================== */
/* Memory Domains (Phase 1D)                                                  */
/* ========================================================================== */

typedef enum {
    BVMM_DOMAIN_NONE             = 0,
    BVMM_DOMAIN_VRAM             = (1 << 0),  /* Dedicated On-Card Video RAM */
    BVMM_DOMAIN_GTT              = (1 << 1),  /* Host RAM bound to GPU aperture */
    BVMM_DOMAIN_SYSTEM           = (1 << 2),  /* Unbound Host System RAM (Staging) */
    BVMM_DOMAIN_UPLOAD           = (1 << 3),  /* Write-Combining Host RAM */
    BVMM_DOMAIN_READBACK         = (1 << 4)   /* CPU Cacheable Host RAM */
} bvmm_domain_t;

/* ========================================================================== */
/* Memory Pool Types (Phase 3 Spec)                                          */
/* ========================================================================== */

typedef enum {
    BVMM_POOL_SMALL              = 0,  /* < 64 KB (Cursors, Queries, Metadata) */
    BVMM_POOL_MEDIUM             = 1,  /* 64 KB - 4 MB (Sprites, Offscreen Textures) */
    BVMM_POOL_LARGE              = 2,  /* 4 MB - 64 MB (Window Surfaces) */
    BVMM_POOL_HUGE               = 3,  /* > 64 MB (4K/8K Scanout, Frame Ring Buffers) */
    BVMM_POOL_TRANSIENT          = 4,  /* Frame-Lifetime Command Streams */
    BVMM_POOL_UPLOAD             = 5,  /* CPU Write -> GPU Read Staging */
    BVMM_POOL_READBACK           = 6,  /* GPU Write -> CPU Read Staging */
    BVMM_POOL_COUNT              = 7
} bvmm_pool_type_t;

/* ========================================================================== */
/* Allocation Flags & States                                                  */
/* ========================================================================== */

typedef enum {
    BVMM_ALLOC_FLAG_NONE         = 0,
    BVMM_ALLOC_FLAG_SCANOUT      = (1 << 0),  /* Hardware Scanout Surface (CRTC) */
    BVMM_ALLOC_FLAG_PINNED       = (1 << 1),  /* Cannot be evicted to GTT */
    BVMM_ALLOC_FLAG_CPU_MAP      = (1 << 2),  /* Must be CPU accessible via BAR */
    BVMM_ALLOC_FLAG_EVICTABLE    = (1 << 3),  /* May be evicted under memory pressure */
    BVMM_ALLOC_FLAG_ZERO_INIT    = (1 << 4),  /* Zero-fill payload on allocation */
    BVMM_ALLOC_FLAG_SPARSE       = (1 << 5)   /* Virtual page binding (Vulkan sparse) */
} bvmm_alloc_flags_t;

typedef enum {
    BVMM_STATE_UNINITIALIZED     = 0,
    BVMM_STATE_RESIDENT          = 1,  /* Payload is currently in VRAM/GTT */
    BVMM_STATE_EVICTED           = 2,  /* Payload is swapped out to System RAM */
    BVMM_STATE_RECLAIM_PENDING   = 3,  /* Pending deferred free after GPU fence */
    BVMM_STATE_ZOMBIE            = 4   /* Application destroyed; retained by Compositor */
} bvmm_alloc_state_t;

/* ========================================================================== */
/* Handle Encoding & Decoding (Phase 1C)                                      */
/* ========================================================================== */

/**
 * 64-bit Production Handle Layout:
 * Bits [63..48] : 16-bit Generation ID (Anti-Double Free Protection)
 * Bits [47..32] : 16-bit Domain & Pool Classification Flags
 * Bits [31..00] : 32-bit Handle Table Index
 */
typedef uint64_t bvmm_handle_t;

#define BVMM_INVALID_HANDLE           ((bvmm_handle_t)0)
#define BVMM_HANDLE_GEN_SHIFT         48
#define BVMM_HANDLE_GEN_MASK          0xFFFF000000000000ULL
#define BVMM_HANDLE_FLAGS_SHIFT       32
#define BVMM_HANDLE_FLAGS_MASK        0x0000FFFF00000000ULL
#define BVMM_HANDLE_INDEX_MASK        0x00000000FFFFFFFFULL

#define BVMM_MAKE_HANDLE(gen, flags, idx) \
    ((((uint64_t)(gen) & 0xFFFFULL) << BVMM_HANDLE_GEN_SHIFT) | \
     (((uint64_t)(flags) & 0xFFFFULL) << BVMM_HANDLE_FLAGS_SHIFT) | \
     ((uint64_t)(idx) & BVMM_HANDLE_INDEX_MASK))

#define BVMM_HANDLE_GET_GEN(h)    ((uint16_t)(((h) & BVMM_HANDLE_GEN_MASK) >> BVMM_HANDLE_GEN_SHIFT))
#define BVMM_HANDLE_GET_FLAGS(h)  ((uint16_t)(((h) & BVMM_HANDLE_FLAGS_MASK) >> BVMM_HANDLE_FLAGS_SHIFT))
#define BVMM_HANDLE_GET_INDEX(h)  ((uint32_t)((h) & BVMM_HANDLE_INDEX_MASK))

/* ========================================================================== */
/* Allocation Request & Container Descriptors                                */
/* ========================================================================== */

typedef struct {
    size_t               size_bytes;         /* Requested byte size */
    size_t               alignment_bytes;    /* Alignment requirement (256, 4096, 65536) */
    bvmm_domain_t        preferred_domain;   /* Preferred domain placement */
    bvmm_domain_t        allowed_domains;    /* Mask of fallback domains */
    bvmm_pool_type_t     pool_type;          /* Target memory pool */
    uint32_t             alloc_flags;        /* Placement/Eviction flags */
    uint32_t             owner_pid;          /* Owning Process ID */
    
    /* 2D Surface Attributes (Optional, for Surfaces/Textures) */
    uint32_t             width;              /* Surface width in pixels */
    uint32_t             height;             /* Surface height in pixels */
    uint32_t             stride_bytes;       /* Hardware line pitch in bytes */
    uint32_t             format_fourcc;      /* Color format FourCC (ARGB8888, etc.) */
} bvmm_alloc_info_t;

typedef struct bvmm_allocation {
    bvmm_handle_t        handle;             /* Unique global handle */
    uint64_t             vram_offset;        /* Physical GPU VRAM or GTT Base Address */
    void*                cpu_virtual_addr;   /* Mapped CPU virtual pointer (if mapped) */
    size_t               size_bytes;         /* Total allocated size including padding */
    size_t               alignment_bytes;    /* Applied physical alignment */
    
    bvmm_domain_t        current_domain;     /* Active memory domain */
    bvmm_pool_type_t     pool_type;          /* Member pool */
    bvmm_alloc_state_t   state;              /* Current residency state */
    uint32_t             alloc_flags;        /* Allocation operational flags */
    
    /* Dual-Domain Reference Counters */
    uint32_t             cpu_refcount;       /* CPU handles open */
    uint32_t             gpu_refcount;       /* GPU pipeline contexts executing */
    
    void*                active_fence;       /* Attached hardware timeline fence */
    uint32_t             owner_pid;          /* Owner process identifier */
    uint16_t             generation_id;      /* Handle generation ID */
    uint64_t             creation_timestamp; /* Timestamp of allocation */
    
    /* Surface Metrics */
    uint32_t             width;
    uint32_t             height;
    uint32_t             stride_bytes;
    uint32_t             format_fourcc;
    
    struct bvmm_allocation* next;            /* Pool linked list link */
    struct bvmm_allocation* prev;
} bvmm_allocation_t;

/* ========================================================================== */
/* Physical Heap & Pool Descriptors                                          */
/* ========================================================================== */

struct bvmm_allocator; /* Forward declaration */

typedef struct {
    uint32_t             heap_id;
    bvmm_domain_t        domain;
    uint64_t             base_address;
    size_t               total_size_bytes;
    size_t               used_size_bytes;
    size_t               free_size_bytes;
    size_t               largest_free_block_bytes;
    struct bvmm_allocator* allocator_ops;    /* Vtable for Range Allocator / TLSF */
    void*                private_data;       /* Allocator internal state */
} bvmm_heap_t;

typedef struct {
    bvmm_pool_type_t     pool_type;
    bvmm_domain_t        domain;
    size_t               min_object_size;
    size_t               max_object_size;
    size_t               total_allocated_bytes;
    uint32_t             object_count;
    bvmm_heap_t*         parent_heap;
    bvmm_allocation_t*   head;
} bvmm_pool_t;

/* ========================================================================== */
/* Telemetry Statistics Container (Phase 14 Forensics)                         */
/* ========================================================================== */

typedef struct {
    size_t               total_vram_bytes;
    size_t               used_vram_bytes;
    size_t               free_vram_bytes;
    size_t               total_gtt_bytes;
    size_t               used_gtt_bytes;
    size_t               free_gtt_bytes;
    size_t               largest_free_vram_block;
    
    uint32_t             active_allocations_count;
    uint32_t             surface_count;
    uint32_t             texture_count;
    uint32_t             cursor_count;
    
    uint32_t             eviction_events_count;
    size_t               evicted_bytes_total;
    uint32_t             defrag_migrations_count;
    uint32_t             allocation_failures_count;
    size_t               peak_vram_watermark_bytes;
} bvmm_stats_t;

/* ========================================================================== */
/* Range Allocator Vtable Interface                                           */
/* ========================================================================== */

typedef struct bvmm_allocator {
    bvmm_result_t (*init)(bvmm_heap_t* heap);
    bvmm_result_t (*allocate)(bvmm_heap_t* heap, size_t size, size_t align, uint64_t* out_offset);
    bvmm_result_t (*free)(bvmm_heap_t* heap, uint64_t offset, size_t size);
    bvmm_result_t (*query_largest_free)(bvmm_heap_t* heap, size_t* out_largest);
    void          (*destroy)(bvmm_heap_t* heap);
} bvmm_allocator_t;

#ifdef __cplusplus
}
#endif

#endif /* _BOS_BVMM_TYPES_H_ */
