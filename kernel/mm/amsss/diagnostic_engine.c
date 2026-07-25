#include "amsss_engine.h"

bool amsss_budget_allows(size_t bytes) {
  uint64_t total = 0, used = 0, free_bytes = 0;
  amsss_heap_backend_stats(&total, &used, &free_bytes);
  (void)total;
  (void)free_bytes;
  return bytes <= g_amsss.budget_bytes && used <= g_amsss.budget_bytes - bytes;
}

void amsss_telemetry_alloc(size_t bytes, bool success) {
  g_amsss.telemetry.allocations++;
  if (success)
    g_amsss.telemetry.bytes_allocated += bytes;
  else
    g_amsss.telemetry.failures++;
}

void amsss_telemetry_free(size_t bytes) {
  g_amsss.telemetry.frees++;
  g_amsss.telemetry.bytes_freed += bytes;
}

amsss_status_t amsss_set_budget(uint64_t budget_bytes) {
  if (budget_bytes == 0 || budget_bytes > AMSSS_HEAP_MAX_BYTES)
    return AMSSS_ERR_INVALID;
  g_amsss.budget_bytes = budget_bytes;
  return AMSSS_OK;
}

amsss_status_t amsss_get_telemetry(amsss_telemetry_t *out) {
  if (!out)
    return AMSSS_ERR_INVALID;
  *out = g_amsss.telemetry;
  return AMSSS_OK;
}

void amsss_optimize(void) {}
void amsss_forensics_record(uint32_t code, uint64_t value) {
  uint32_t slot = g_amsss.event_cursor++ % AMSSS_MAX_EVENTS;
  g_amsss.events[slot].code = code;
  g_amsss.events[slot].value = value;
}
void amsss_dump_diagnostics(void) {}

amsss_status_t
amsss_register_reclaim_provider(const amsss_reclaim_provider_t *provider) {
  if (!provider || !provider->reclaim || !provider->name)
    return AMSSS_ERR_INVALID;
  for (uint32_t i = 0; i < AMSSS_MAX_PROVIDERS; ++i) {
    if (!g_amsss.providers[i].active) {
      g_amsss.providers[i] = *provider;
      g_amsss.providers[i].active = true;
      g_amsss.reclaimable_bytes += provider->bytes;
      return AMSSS_OK;
    }
  }
  return AMSSS_ERR_LIMIT;
}

size_t amsss_reclaim(size_t target_bytes) {
  size_t reclaimed = 0;
  for (uint32_t i = 0; i < AMSSS_MAX_PROVIDERS && reclaimed < target_bytes;
       ++i) {
    if (g_amsss.providers[i].active) {
      size_t released = g_amsss.providers[i].reclaim(
          g_amsss.providers[i].context, target_bytes - reclaimed);
      if (released > target_bytes - reclaimed)
        released = target_bytes - reclaimed;
      reclaimed += released;
    }
  }
  g_amsss.telemetry.reclaim_runs++;
  if (reclaimed >= g_amsss.reclaimable_bytes)
    g_amsss.reclaimable_bytes = 0;
  else
    g_amsss.reclaimable_bytes -= reclaimed;
  return reclaimed;
}

size_t amsss_cache_reclaim(size_t target_bytes) {
  size_t reclaimed = 0;
  for (uint32_t i = 0; i < AMSSS_MAX_CACHES && reclaimed < target_bytes; ++i) {
    amsss_cache_entry_t *entry = &g_amsss.caches[i];
    if (!entry->active || entry->ref_count != 0 || !entry->destroy)
      continue;
    entry->destroy(entry->object, entry->context);
    if (entry->bytes <= target_bytes - reclaimed)
      reclaimed += entry->bytes;
    else
      reclaimed = target_bytes;
    entry->active = false;
  }
  return reclaimed;
}

amsss_status_t amsss_register_cache(const amsss_cache_entry_t *entry) {
  if (!entry || !entry->name || !entry->destroy)
    return AMSSS_ERR_INVALID;
  for (uint32_t i = 0; i < AMSSS_MAX_CACHES; ++i) {
    if (!g_amsss.caches[i].active) {
      g_amsss.caches[i] = *entry;
      g_amsss.caches[i].active = true;
      g_amsss.reclaimable_bytes += entry->bytes;
      return AMSSS_OK;
    }
  }
  return AMSSS_ERR_LIMIT;
}
