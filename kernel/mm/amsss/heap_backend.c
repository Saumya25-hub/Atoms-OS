#include "../../core/memory/heap/include/heap.h"
#include "amsss.h"

static size_t amsss_align_up(size_t value, size_t alignment) {
  return (value + alignment - 1) & ~(alignment - 1);
}

void *amsss_heap_backend_alloc(size_t size, uint64_t caller_rip) {
  return heap_alloc_raw(size, caller_rip);
}

void *amsss_heap_backend_aligned_alloc(size_t size, size_t alignment,
                                       uint64_t caller_rip) {
  if (size == 0)
    return NULL;
  if (alignment < sizeof(void *) || (alignment & (alignment - 1)) != 0)
    alignment = sizeof(void *);
  if (size > (size_t)-1 - alignment - sizeof(void *))
    return NULL;

  size_t total = size + alignment + sizeof(void *);
  void *raw = heap_alloc_raw(total, caller_rip);
  if (!raw)
    return NULL;

  uintptr_t aligned =
      amsss_align_up((uintptr_t)raw + sizeof(void *), alignment);
  ((void **)aligned)[-1] = raw;
  return (void *)aligned;
}

void *amsss_heap_backend_realloc(void *ptr, size_t size, uint64_t caller_rip) {
  if (!ptr)
    return heap_alloc_raw(size, caller_rip);
  return heap_realloc_raw(ptr, size, caller_rip);
}

void amsss_heap_backend_free(void *ptr) {
  if (!ptr)
    return;
  heap_free_raw(ptr);
}

amsss_status_t amsss_heap_expand(size_t minimum_bytes) {
  return heap_expand_raw(minimum_bytes) ? AMSSS_OK : AMSSS_ERR_NO_MEMORY;
}

void amsss_heap_backend_stats(uint64_t *total, uint64_t *used,
                              uint64_t *free_bytes) {
  HeapStats stats;
  heap_get_stats(&stats);
  if (total)
    *total = stats.total_size;
  if (used)
    *used = stats.used_size;
  if (free_bytes)
    *free_bytes = stats.free_size;
}
