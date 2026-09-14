/*
 * ATOMS OS — Freestanding Portability Header for Audio Codecs
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 */

#ifndef BOS_AUDIO_PORTABILITY_H
#define BOS_AUDIO_PORTABILITY_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef BOS_HOST_TEST
#include <stdlib.h>
#include <string.h>

static inline void* bos_audio_malloc(size_t sz) {
    return malloc(sz);
}

static inline void* bos_audio_realloc(void* ptr, size_t sz) {
    return realloc(ptr, sz);
}

static inline void bos_audio_free(void* ptr) {
    if (ptr) {
        free(ptr);
    }
}

static inline void* bos_audio_calloc(size_t num, size_t sz) {
    return calloc(num, sz);
}
#else
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"

/* Memory Allocation Abstraction */
static inline void* bos_audio_malloc(size_t sz) {
    return kmalloc(sz);
}

static inline void* bos_audio_realloc(void* ptr, size_t sz) {
    return krealloc(ptr, sz);
}

static inline void bos_audio_free(void* ptr) {
    if (ptr) {
        kfree(ptr);
    }
}

static inline void* bos_audio_calloc(size_t num, size_t sz) {
    size_t total = num * sz;
    void* p = kmalloc(total);
    if (p) {
        memset(p, 0, total);
    }
    return p;
}
#endif

#ifndef assert
#define assert(expr) ((void)0)
#endif

#endif /* BOS_AUDIO_PORTABILITY_H */
