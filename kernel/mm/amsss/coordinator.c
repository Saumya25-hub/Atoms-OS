#include "amsss_engine.h"

extern void display_print(const char *text);

amsss_state_t g_amsss;

static void amsss_zero_state(void) {
  uint8_t *p = (uint8_t *)&g_amsss;
  for (size_t i = 0; i < sizeof(g_amsss); ++i)
    p[i] = 0;
}

void amsss_init(void) {
  amsss_zero_state();
  g_amsss.budget_bytes = AMSSS_HEAP_MAX_BYTES;
  g_amsss.initialized = true;
  amsss_pressure_init();
  amsss_health_refresh();
  display_print("[AMSSS v2] coordinator ready\n");
}

amsss_status_t amsss_register_defaults(void) {
  extern void image_cache_init(void);
  extern size_t image_cache_reclaim_registered(void *context,
                                               size_t target_bytes);
  static bool registered = false;
  static const amsss_reclaim_provider_t image_cache_provider = {
      .name = "bopawn-image-cache",
      .reclaim = image_cache_reclaim_registered,
      .context = NULL,
      .bytes = 0,
      .active = false};

  if (!g_amsss.initialized)
    return AMSSS_ERR_NOT_READY;
  if (registered)
    return AMSSS_OK;

  image_cache_init();
  amsss_status_t status =
      amsss_register_reclaim_provider(&image_cache_provider);
  if (status == AMSSS_OK)
    registered = true;
  return status;
}

bool amsss_is_initialized(void) { return g_amsss.initialized; }

void *amsss_alloc(size_t size, size_t alignment, uint64_t caller_rip) {
  if (size == 0)
    return NULL;
  if (!g_amsss.initialized || g_amsss.allocation_active)
    return alignment > sizeof(void *)
               ? amsss_heap_backend_aligned_alloc(size, alignment, caller_rip)
               : amsss_heap_backend_alloc(size, caller_rip);
  g_amsss.allocation_active = true;
  if (!amsss_budget_allows(size))
    amsss_reclaim(size);
  void *result =
      alignment > sizeof(void *)
          ? amsss_heap_backend_aligned_alloc(size, alignment, caller_rip)
          : amsss_heap_backend_alloc(size, caller_rip);
  if (!result) {
    amsss_reclaim(size);
    result = alignment > sizeof(void *)
                 ? amsss_heap_backend_aligned_alloc(size, alignment, caller_rip)
                 : amsss_heap_backend_alloc(size, caller_rip);
  }
  amsss_telemetry_alloc(size, result != NULL);
  g_amsss.allocation_active = false;
  amsss_pressure_update();
  return result;
}

void *amsss_calloc(size_t count, size_t size, uint64_t caller_rip) {
  if (count && size > ((size_t)-1) / count)
    return NULL;
  size_t bytes = count * size;
  uint8_t *p = (uint8_t *)amsss_alloc(bytes, sizeof(void *), caller_rip);
  if (p)
    for (size_t i = 0; i < bytes; ++i)
      p[i] = 0;
  return p;
}

void *amsss_realloc(void *ptr, size_t size, uint64_t caller_rip) {
  if (!g_amsss.initialized || g_amsss.allocation_active)
    return amsss_heap_backend_realloc(ptr, size, caller_rip);
  g_amsss.allocation_active = true;
  void *result = amsss_heap_backend_realloc(ptr, size, caller_rip);
  if (!result && size) {
    amsss_reclaim(size);
    result = amsss_heap_backend_realloc(ptr, size, caller_rip);
  }
  amsss_telemetry_alloc(size, result != NULL || size == 0);
  g_amsss.allocation_active = false;
  amsss_pressure_update();
  return result;
}

void amsss_free(void *ptr) {
  amsss_heap_backend_free(ptr);
  if (g_amsss.initialized)
    g_amsss.telemetry.frees++;
}

void *amsss_raw_alloc(size_t size, size_t alignment) {
  return alignment > sizeof(void *)
             ? amsss_heap_backend_aligned_alloc(size, alignment, 0)
             : amsss_heap_backend_alloc(size, 0);
}

void amsss_raw_free(void *ptr, size_t size) {
  (void)size;
  amsss_heap_backend_free(ptr);
}

void amsss_tick(void) {
  if (!g_amsss.initialized)
    return;
  amsss_health_refresh();
  amsss_pressure_update();
  amsss_optimize();
}
