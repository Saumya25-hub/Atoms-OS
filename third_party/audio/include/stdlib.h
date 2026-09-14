#ifndef _BOS_AUDIO_STDLIB_H
#define _BOS_AUDIO_STDLIB_H

#include <stddef.h>
#include <stdint.h>
#include "kernel/core/memory/heap/include/heap.h"

static inline void* malloc(size_t sz) {
    return kmalloc(sz);
}

static inline void* realloc(void* p, size_t sz) {
    return krealloc(p, sz);
}

static inline void free(void* p) {
    if (p) {
        kfree(p);
    }
}

static inline int abs(int x) {
    return (x < 0) ? -x : x;
}

#endif /* _BOS_AUDIO_STDLIB_H */
