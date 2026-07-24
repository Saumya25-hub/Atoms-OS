#ifndef ABE_DIAGNOSTICS_H
#define ABE_DIAGNOSTICS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Diagnostics & Performance Metrics Subsystem
typedef struct {
    uint32_t fps;
    uint32_t dom_nodes;
    uint32_t layout_time_us;
    uint32_t paint_time_us;
    uint32_t parse_time_us;
    uint32_t js_exec_time_us;
    uint32_t memory_used_kb;
    uint32_t cache_hit_rate_pct;
} ABE_DiagnosticsMetrics;

void                   ABE_Diagnostics_Init(void);
ABE_DiagnosticsMetrics ABE_Diagnostics_GetMetrics(void);

#ifdef __cplusplus
}
#endif

#endif // ABE_DIAGNOSTICS_H
