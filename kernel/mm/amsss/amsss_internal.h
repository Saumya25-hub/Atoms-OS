#ifndef AMSSS_INTERNAL_H
#define AMSSS_INTERNAL_H

#include "amsss.h"

typedef struct {
  bool initialized;
  bool allocation_active;
  uint64_t budget_bytes;
  uint64_t peak_bytes;
  uint64_t reclaimable_bytes;
  uint32_t corruption_events;
  amsss_reclaim_provider_t providers[AMSSS_MAX_PROVIDERS];
  amsss_cache_entry_t caches[AMSSS_MAX_CACHES];
  amsss_telemetry_t telemetry;
  struct {
    uint32_t code;
    uint64_t value;
  } events[AMSSS_MAX_EVENTS];
  uint32_t event_cursor;
} amsss_state_t;

extern amsss_state_t g_amsss;

void amsss_pressure_update(void);
void amsss_health_refresh(void);
bool amsss_budget_allows(size_t bytes);
void amsss_telemetry_alloc(size_t bytes, bool success);
void amsss_telemetry_free(size_t bytes);
size_t amsss_cache_reclaim(size_t target_bytes);
void amsss_optimize(void);

#endif
