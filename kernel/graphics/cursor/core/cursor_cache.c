/**
 * @file cursor_cache.c
 * @brief LRU Cache & Reference Counter Engine Implementation for BCE V1.0
 */

#include "../include/bos_cursor_cache.h"
#include "../include/bos_cur_loader.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/debug/step14_telemetry.h"

extern void* kmalloc(size_t size);
extern void kfree(void* ptr);

static bce_cache_t g_cursor_cache = {0};

bce_error_t bos_cursor_cache_init(void) {
    memset(&g_cursor_cache, 0, sizeof(bce_cache_t));
    return BCE_OK;
}

void bos_cursor_cache_shutdown(void) {
    bce_cache_node_t* curr = g_cursor_cache.head;
    while (curr) {
        bce_cache_node_t* next = curr->next;
        if (curr->cursor) {
            bos_cur_free(curr->cursor);
        }
        kfree(curr);
        curr = next;
    }
    memset(&g_cursor_cache, 0, sizeof(bce_cache_t));
}

bce_cursor_t* bos_cursor_cache_get(uint32_t key_hash) {
    bce_cache_node_t* curr = g_cursor_cache.head;
    while (curr) {
        if (curr->key_hash == key_hash && curr->cursor) {
            g_cursor_cache.hits++;
            curr->ref_count++;
            curr->last_accessed_us = step14_rdtsc();
            return curr->cursor;
        }
        curr = curr->next;
    }
    g_cursor_cache.misses++;
    return NULL;
}

bce_error_t bos_cursor_cache_put(uint32_t key_hash, bce_cursor_t* cursor) {
    if (!cursor) return BCE_ERR_INVALID_PARAM;

    /* Evict oldest LRU node if capacity reached */
    if (g_cursor_cache.count >= BCE_CACHE_MAX_CAPACITY && g_cursor_cache.tail) {
        bce_cache_node_t* oldest = g_cursor_cache.tail;
        if (oldest->prev) oldest->prev->next = NULL;
        g_cursor_cache.tail = oldest->prev;
        if (oldest->cursor) bos_cur_free(oldest->cursor);
        kfree(oldest);
        g_cursor_cache.count--;
    }

    bce_cache_node_t* node = (bce_cache_node_t*)kmalloc(sizeof(bce_cache_node_t));
    if (!node) return BCE_ERR_NO_MEMORY;
    memset(node, 0, sizeof(bce_cache_node_t));

    node->key_hash = key_hash;
    node->cursor = cursor;
    node->ref_count = 1;
    node->last_accessed_us = step14_rdtsc();

    node->next = g_cursor_cache.head;
    if (g_cursor_cache.head) g_cursor_cache.head->prev = node;
    g_cursor_cache.head = node;
    if (!g_cursor_cache.tail) g_cursor_cache.tail = node;

    g_cursor_cache.count++;
    return BCE_OK;
}

void bos_cursor_cache_release(bce_cursor_t* cursor) {
    if (!cursor) return;
    bce_cache_node_t* curr = g_cursor_cache.head;
    while (curr) {
        if (curr->cursor == cursor) {
            if (curr->ref_count > 0) curr->ref_count--;
            break;
        }
        curr = curr->next;
    }
}

void bos_cursor_cache_get_stats(uint32_t* out_hits, uint32_t* out_misses, uint32_t* out_count) {
    if (out_hits) *out_hits = g_cursor_cache.hits;
    if (out_misses) *out_misses = g_cursor_cache.misses;
    if (out_count) *out_count = g_cursor_cache.count;
}
