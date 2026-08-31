/*
 * ATOMS OS — Userspace Dynamic Memory Allocator (malloc/free)
 * Adapted from dlmalloc / TLSF principles (Public Domain / MIT)
 * Operates purely on top of mmap / munmap anonymous page allocations.
 */

#include "../include/stdlib.h"
#include "../include/sys/mman.h"
#include "../include/string.h"

#define CHUNK_MAGIC_ALLOC 0x4D414C43ULL /* "MALC" */
#define CHUNK_MAGIC_FREE  0x46524545ULL /* "FREE" */
#define HEAP_PAGE_SIZE    4096ULL
#define ARENA_CHUNK_SIZE  (64 * 1024)   /* 64KB initial arena */

typedef struct ChunkHeader {
    uint64_t magic;
    size_t   size;       /* Usable payload size */
    struct ChunkHeader *next;
    struct ChunkHeader *prev;
    int is_free;
} ChunkHeader;

static ChunkHeader *s_free_list_head = NULL;

static void *allocate_new_arena(size_t min_payload) {
    size_t total_needed = sizeof(ChunkHeader) + min_payload;
    size_t pages = (total_needed + HEAP_PAGE_SIZE - 1) / HEAP_PAGE_SIZE;
    size_t alloc_bytes = pages * HEAP_PAGE_SIZE;
    if (alloc_bytes < ARENA_CHUNK_SIZE) {
        alloc_bytes = ARENA_CHUNK_SIZE;
    }

    void *mem = mmap(NULL, alloc_bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) {
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

    /* Check free list for first-fit chunk */
    ChunkHeader *curr = s_free_list_head;
    while (curr) {
        if (curr->is_free && curr->size >= size) {
            /* If chunk is significantly larger, split it */
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
            return (void *)(curr + 1);
        }
        curr = curr->next;
    }

    /* No suitable free chunk found; allocate new arena from OS */
    ChunkHeader *new_chunk = (ChunkHeader *)allocate_new_arena(size);
    if (!new_chunk) return NULL;

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

    return (void *)(new_chunk + 1);
}

void free(void *ptr) {
    if (!ptr) return;

    ChunkHeader *chunk = ((ChunkHeader *)ptr) - 1;
    if (chunk->magic != CHUNK_MAGIC_ALLOC) {
        return; /* Invalid pointer or double free prevention */
    }

    chunk->is_free = 1;
    chunk->magic = CHUNK_MAGIC_FREE;

    /* Coalesce with next adjacent chunk if free */
    if (chunk->next && chunk->next->is_free && ((uint8_t *)(chunk + 1) + chunk->size) == (uint8_t *)chunk->next) {
        chunk->size += sizeof(ChunkHeader) + chunk->next->size;
        chunk->next = chunk->next->next;
        if (chunk->next) chunk->next->prev = chunk;
    }

    /* Coalesce with previous adjacent chunk if free */
    if (chunk->prev && chunk->prev->is_free && ((uint8_t *)(chunk->prev + 1) + chunk->prev->size) == (uint8_t *)chunk) {
        chunk->prev->size += sizeof(ChunkHeader) + chunk->size;
        chunk->prev->next = chunk->next;
        if (chunk->next) chunk->next->prev = chunk->prev;
    }
}

void *calloc(size_t nmemb, size_t size) {
    size_t total = nmemb * size;
    if (nmemb != 0 && total / nmemb != size) return NULL; /* Overflow */
    void *ptr = malloc(total);
    if (ptr) {
        memset(ptr, 0, total);
    }
    return ptr;
}

void *realloc(void *ptr, size_t size) {
    if (!ptr) return malloc(size);
    if (size == 0) {
        free(ptr);
        return NULL;
    }

    ChunkHeader *chunk = ((ChunkHeader *)ptr) - 1;
    if (chunk->magic != CHUNK_MAGIC_ALLOC) return NULL;

    if (chunk->size >= size) {
        return ptr; /* Already large enough */
    }

    void *new_ptr = malloc(size);
    if (!new_ptr) return NULL;

    memcpy(new_ptr, ptr, chunk->size);
    free(ptr);
    return new_ptr;
}

