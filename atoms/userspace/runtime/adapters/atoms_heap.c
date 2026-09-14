/*
 * ATOMS OS — Userspace Dynamic Heap Allocator
 * Implements standard malloc/free/calloc/realloc on top of ATOMS sys_mmap.
 * Features 16-byte alignment, chunk splitting, free-list coalescing, and spinlock protection.
 */

#include "atoms/userspace/runtime/include/atoms_syscall.h"
#include <stdint.h>
#include <stddef.h>

#define CHUNK_MAGIC_ALLOC 0x4D414C43ULL /* "MALC" */
#define CHUNK_MAGIC_FREE  0x46524545ULL /* "FREE" */
#define HEAP_PAGE_SIZE    4096ULL
#define ARENA_CHUNK_SIZE  (64 * 1024)   /* 64KB arena granularity */

typedef struct ChunkHeader {
    uint64_t magic;
    size_t   size;       /* Usable payload size */
    struct ChunkHeader *next;
    struct ChunkHeader *prev;
    int is_free;
    uint32_t padding;
    uint64_t align_pad;  /* Enforce 48-byte header (multiple of 16) for 16-byte payload alignment */
} __attribute__((aligned(16))) ChunkHeader;

static ChunkHeader *s_free_list_head = NULL;
static volatile int s_heap_lock = 0;

static inline void heap_lock_acquire(void) {
    while (__atomic_test_and_set(&s_heap_lock, __ATOMIC_ACQUIRE)) {
        atoms_sys_futex((uint32_t *)&s_heap_lock, FUTEX_WAIT, 1, NULL);
    }
}

static inline void heap_lock_release(void) {
    __atomic_clear(&s_heap_lock, __ATOMIC_RELEASE);
    atoms_sys_futex((uint32_t *)&s_heap_lock, FUTEX_WAKE, 1, NULL);
}

void atoms_heap_init(void) {
    s_free_list_head = NULL;
    s_heap_lock = 0;
}

static void *allocate_new_arena(size_t min_payload) {
    size_t total_needed = sizeof(ChunkHeader) + min_payload;
    size_t pages = (total_needed + HEAP_PAGE_SIZE - 1) / HEAP_PAGE_SIZE;
    size_t alloc_bytes = pages * HEAP_PAGE_SIZE;
    if (alloc_bytes < ARENA_CHUNK_SIZE) {
        alloc_bytes = ARENA_CHUNK_SIZE;
    }

    void *mem = atoms_sys_mmap(NULL, alloc_bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == (void *)-1 || !mem) {
        return NULL;
    }

    ChunkHeader *chunk = (ChunkHeader *)mem;
    chunk->magic = CHUNK_MAGIC_ALLOC;
    chunk->size = alloc_bytes - sizeof(ChunkHeader);
    chunk->next = NULL;
    chunk->prev = NULL;
    chunk->is_free = 0;

    return chunk;
}

void *malloc(size_t size) {
    if (size == 0) return NULL;

    /* 16-byte alignment */
    size = (size + 15) & ~15ULL;

    heap_lock_acquire();

    /* Search free list for first-fit block */
    ChunkHeader *curr = s_free_list_head;
    while (curr) {
        if (curr->is_free && curr->size >= size) {
            /* If chunk is large enough to split, split it */
            if (curr->size >= size + sizeof(ChunkHeader) + 32) {
                size_t remainder = curr->size - size - sizeof(ChunkHeader);
                ChunkHeader *split = (ChunkHeader *)((uint8_t *)(curr + 1) + size);
                split->magic = CHUNK_MAGIC_FREE;
                split->size = remainder;
                split->is_free = 1;
                split->next = curr->next;
                split->prev = curr;
                if (curr->next) curr->next->prev = split;
                curr->next = split;
                curr->size = size;
            }
            curr->is_free = 0;
            curr->magic = CHUNK_MAGIC_ALLOC;
            heap_lock_release();
            return (void *)(curr + 1);
        }
        curr = curr->next;
    }

    /* No free chunk available; allocate new arena from OS */
    ChunkHeader *new_chunk = (ChunkHeader *)allocate_new_arena(size);
    if (!new_chunk) {
        heap_lock_release();
        return NULL;
    }

    /* If new chunk is larger than requested, split the remainder onto free list */
    if (new_chunk->size >= size + sizeof(ChunkHeader) + 32) {
        size_t remainder = new_chunk->size - size - sizeof(ChunkHeader);
        ChunkHeader *split = (ChunkHeader *)((uint8_t *)(new_chunk + 1) + size);
        split->magic = CHUNK_MAGIC_FREE;
        split->size = remainder;
        split->is_free = 1;
        split->next = s_free_list_head;
        split->prev = NULL;
        if (s_free_list_head) s_free_list_head->prev = split;
        s_free_list_head = split;

        new_chunk->size = size;
    }

    new_chunk->next = s_free_list_head;
    new_chunk->prev = NULL;
    if (s_free_list_head) s_free_list_head->prev = new_chunk;
    s_free_list_head = new_chunk;

    heap_lock_release();
    return (void *)(new_chunk + 1);
}

void free(void *ptr) {
    if (!ptr) return;

    heap_lock_acquire();

    ChunkHeader *chunk = (ChunkHeader *)ptr - 1;
    if (chunk->magic != CHUNK_MAGIC_ALLOC) {
        heap_lock_release();
        return; /* Invalid or corrupted pointer */
    }

    chunk->magic = CHUNK_MAGIC_FREE;
    chunk->is_free = 1;

    /* Coalesce with next chunk if free */
    if (chunk->next && chunk->next->is_free) {
        chunk->size += sizeof(ChunkHeader) + chunk->next->size;
        chunk->next = chunk->next->next;
        if (chunk->next) {
            chunk->next->prev = chunk;
        }
    }

    /* Coalesce with prev chunk if free */
    if (chunk->prev && chunk->prev->is_free) {
        chunk->prev->size += sizeof(ChunkHeader) + chunk->size;
        chunk->prev->next = chunk->next;
        if (chunk->next) {
            chunk->next->prev = chunk->prev;
        }
    }

    heap_lock_release();
}

void *calloc(size_t nmemb, size_t size) {
    size_t total = nmemb * size;
    if (nmemb != 0 && total / nmemb != size) return NULL; /* Overflow */

    void *ptr = malloc(total);
    if (!ptr) return NULL;

    uint8_t *p = (uint8_t *)ptr;
    for (size_t i = 0; i < total; i++) {
        p[i] = 0;
    }
    return ptr;
}

void *realloc(void *ptr, size_t size) {
    if (!ptr) return malloc(size);
    if (size == 0) {
        free(ptr);
        return NULL;
    }

    ChunkHeader *chunk = (ChunkHeader *)ptr - 1;
    if (chunk->magic != CHUNK_MAGIC_ALLOC) return NULL;

    if (chunk->size >= size) {
        return ptr; /* Already large enough */
    }

    void *new_ptr = malloc(size);
    if (!new_ptr) return NULL;

    size_t copy_size = chunk->size < size ? chunk->size : size;
    uint8_t *src = (uint8_t *)ptr;
    uint8_t *dst = (uint8_t *)new_ptr;
    for (size_t i = 0; i < copy_size; i++) {
        dst[i] = src[i];
    }

    free(ptr);
    return new_ptr;
}

int posix_memalign(void **memptr, size_t alignment, size_t size) {
    if (!memptr) return -1;
    if ((alignment & (alignment - 1)) != 0 || alignment < sizeof(void *)) {
        return -1;
    }
    /* We align to at least 16 bytes by default */
    void *ptr = malloc(size < alignment ? alignment : size);
    if (!ptr) return -1;
    *memptr = ptr;
    return 0;
}

void *aligned_alloc(size_t alignment, size_t size) {
    void *ptr = NULL;
    if (posix_memalign(&ptr, alignment, size) != 0) return NULL;
    return ptr;
}
