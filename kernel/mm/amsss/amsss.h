#ifndef AMSSS_H
#define AMSSS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define AMSSS_VERSION 2u
#define AMSSS_MAX_PROVIDERS 16u
#define AMSSS_MAX_CACHES 16u
#define AMSSS_MAX_EVENTS 64u
#define AMSSS_PAGE_SIZE 4096u
#define AMSSS_HEAP_MAX_BYTES (256ULL * 1024ULL * 1024ULL)

typedef enum {
  AMSSS_OK = 0,
  AMSSS_ERR_INVALID = -1,
  AMSSS_ERR_NOT_READY = -2,
  AMSSS_ERR_NO_MEMORY = -3,
  AMSSS_ERR_LIMIT = -4,
  AMSSS_ERR_UNSUPPORTED = -5
} amsss_status_t;

typedef enum {
  AMSSS_PRESSURE_NORMAL = 0,
  AMSSS_PRESSURE_ELEVATED = 1,
  AMSSS_PRESSURE_CRITICAL = 2
} amsss_pressure_level_t;

typedef struct {
  uint64_t total_bytes;
  uint64_t used_bytes;
  uint64_t free_bytes;
  uint64_t reclaimable_bytes;
  uint64_t peak_bytes;
  uint32_t allocation_failures;
  uint32_t corruption_events;
  amsss_pressure_level_t pressure;
  bool initialized;
} amsss_health_t;

typedef struct {
  void *(*map_page)(void *context, uint64_t virtual_address, uint32_t flags);
  void (*unmap_page)(void *context, uint64_t virtual_address);
  void *context;
  uint64_t virtual_base;
  uint64_t virtual_limit;
} amsss_vmm_provider_t;

typedef size_t (*amsss_reclaim_fn)(void *context, size_t target_bytes);
typedef void (*amsss_cache_destroy_fn)(void *object, void *context);

typedef struct {
  const char *name;
  amsss_reclaim_fn reclaim;
  void *context;
  uint64_t bytes;
  bool active;
} amsss_reclaim_provider_t;

typedef struct {
  const char *name;
  void *object;
  size_t bytes;
  amsss_cache_destroy_fn destroy;
  void *context;
  uint32_t priority;
  uint32_t ref_count;
  bool active;
} amsss_cache_entry_t;

typedef struct {
  uint64_t allocations;
  uint64_t frees;
  uint64_t bytes_allocated;
  uint64_t bytes_freed;
  uint32_t failures;
  uint32_t expansions;
  uint32_t reclaim_runs;
  uint32_t diagnostic_events;
} amsss_telemetry_t;

void amsss_init(void);
bool amsss_is_initialized(void);
amsss_status_t amsss_register_defaults(void);
void *amsss_alloc(size_t size, size_t alignment, uint64_t caller_rip);
void *amsss_calloc(size_t count, size_t size, uint64_t caller_rip);
void *amsss_realloc(void *ptr, size_t size, uint64_t caller_rip);
void amsss_free(void *ptr);
void *amsss_raw_alloc(size_t size, size_t alignment);
void amsss_raw_free(void *ptr, size_t size);

amsss_status_t
amsss_register_reclaim_provider(const amsss_reclaim_provider_t *provider);
amsss_status_t amsss_register_cache(const amsss_cache_entry_t *entry);
size_t amsss_reclaim(size_t target_bytes);
amsss_pressure_level_t amsss_pressure_level(void);
amsss_status_t amsss_set_budget(uint64_t budget_bytes);
amsss_status_t amsss_get_health(amsss_health_t *out);
amsss_status_t amsss_get_telemetry(amsss_telemetry_t *out);
void amsss_tick(void);
void amsss_dump_diagnostics(void);
void amsss_forensics_record(uint32_t code, uint64_t value);

/* Heap backend hooks. These never call the AMSSS public allocation API. */
void *amsss_heap_backend_alloc(size_t size, uint64_t caller_rip);
void *amsss_heap_backend_aligned_alloc(size_t size, size_t alignment,
                                       uint64_t caller_rip);
void *amsss_heap_backend_realloc(void *ptr, size_t size, uint64_t caller_rip);
void amsss_heap_backend_free(void *ptr);
amsss_status_t amsss_heap_expand(size_t minimum_bytes);
void amsss_heap_backend_stats(uint64_t *total, uint64_t *used,
                              uint64_t *free_bytes);

#endif
