#include "amsss_engine.h"

static amsss_health_t health;

void amsss_health_refresh(void) {
  uint64_t total = 0, used = 0, free_bytes = 0;
  amsss_heap_backend_stats(&total, &used, &free_bytes);
  health.total_bytes = total;
  health.used_bytes = used;
  health.free_bytes = free_bytes;
  health.reclaimable_bytes = g_amsss.reclaimable_bytes;
  if (used > g_amsss.peak_bytes)
    g_amsss.peak_bytes = used;
  health.peak_bytes = g_amsss.peak_bytes;
  health.allocation_failures = g_amsss.telemetry.failures;
  health.corruption_events = g_amsss.corruption_events;
  health.pressure = amsss_pressure_level();
  health.initialized = g_amsss.initialized;
}

amsss_status_t amsss_get_health(amsss_health_t *out) {
  if (!out)
    return AMSSS_ERR_INVALID;
  amsss_health_refresh();
  *out = health;
  return AMSSS_OK;
}
