#ifndef BWE_DIAGNOSTICS_H
#define BWE_DIAGNOSTICS_H

#include "bwe.h"

#define BWE_DIAG_MAX_LOGS 64

typedef struct {
    uint32_t window_id;
    char reason[64];
    uint64_t timestamp;
} BWE_DiagLog;

void BWE_Diag_Init(void);
void BWE_Diag_LogViolation(uint32_t window_id, const char* reason);
uint32_t BWE_Diag_GetLogs(BWE_DiagLog* out_logs, uint32_t max_logs);
void BWE_Diag_ClearLogs(void);

// Geometry Guard: Checks if child bounds exceed parent client bounds
void BWE_Diag_GuardBounds(BWE_Window* parent, BWE_Window* child);

#endif // BWE_DIAGNOSTICS_H
