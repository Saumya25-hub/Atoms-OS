#include "amsss_engine.h"

static amsss_pressure_level_t current_pressure;

void amsss_pressure_init(void) { current_pressure = AMSSS_PRESSURE_NORMAL; }

void amsss_pressure_update(void) {
  uint64_t total = 0, used = 0, free_bytes = 0;
  amsss_heap_backend_stats(&total, &used, &free_bytes);
  if (!total || free_bytes * 100ULL >= total * 25ULL)
    current_pressure = AMSSS_PRESSURE_NORMAL;
  else if (free_bytes * 100ULL >= total * 10ULL)
    current_pressure = AMSSS_PRESSURE_ELEVATED;
  else
    current_pressure = AMSSS_PRESSURE_CRITICAL;
}

amsss_pressure_level_t amsss_pressure_level(void) { return current_pressure; }
