/**
 * @file bos_cursor_cache.h
 * @brief LRU Cache & Reference Counter Engine for BCE V1.0
 */

#ifndef BOS_CURSOR_CACHE_H
#define BOS_CURSOR_CACHE_H

#include "bos_cursor.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BCE_CACHE_MAX_CAPACITY 64

typedef struct bce_cache_node {
    uint32_t key_hash;
    bce_cursor_t* cursor;
    uint32_t ref_count;
    uint64_t last_accessed_us;
    struct bce_cache_node* prev;
    struct bce_cache_node* next;
} bce_cache_node_t;

typedef struct {
    bce_cache_node_t* head;
    bce_cache_node_t* tail;
    uint32_t count;
    uint32_t hits;
    uint32_t misses;
} bce_cache_t;

bce_error_t bos_cursor_cache_init(void);
void        bos_cursor_cache_shutdown(void);
bce_cursor_t* bos_cursor_cache_get(uint32_t key_hash);
bce_error_t bos_cursor_cache_put(uint32_t key_hash, bce_cursor_t* cursor);
void        bos_cursor_cache_release(bce_cursor_t* cursor);
void        bos_cursor_cache_get_stats(uint32_t* out_hits, uint32_t* out_misses, uint32_t* out_count);

#ifdef __cplusplus
}
#endif

#endif /* BOS_CURSOR_CACHE_H */
