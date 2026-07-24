#ifndef ATRIX_BROWSER_NAVIGATION_H
#define ATRIX_BROWSER_NAVIGATION_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// ATRIX Navigation Engine Subsystem
// ============================================================

#define ATRIX_NAV_HISTORY_MAX 32

typedef struct {
    char     history_urls[ATRIX_NAV_HISTORY_MAX][128];
    int32_t  current_index;
    int32_t  total_entries;
    bool     is_loading;
    uint32_t last_status_code;
} ATRIX_NavigationState;

void                  ATRIX_Navigation_Init(void);
bool                  ATRIX_Navigation_Navigate(const char* url);
bool                  ATRIX_Navigation_Back(void);
bool                  ATRIX_Navigation_Forward(void);
bool                  ATRIX_Navigation_Reload(void);
bool                  ATRIX_Navigation_Stop(void);
bool                  ATRIX_Navigation_Home(void);
ATRIX_NavigationState* ATRIX_Navigation_GetState(void);

#ifdef __cplusplus
}
#endif

#endif // ATRIX_BROWSER_NAVIGATION_H
