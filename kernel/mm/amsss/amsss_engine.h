#ifndef AMSSS_ENGINE_H
#define AMSSS_ENGINE_H

#include "amsss_internal.h"

void amsss_pressure_init(void);
void amsss_pressure_update(void);
void amsss_health_refresh(void);
bool amsss_budget_allows(size_t bytes);
void amsss_telemetry_alloc(size_t bytes, bool success);
void amsss_telemetry_free(size_t bytes);
size_t amsss_cache_reclaim(size_t target_bytes);
void amsss_optimize(void);

#endif
