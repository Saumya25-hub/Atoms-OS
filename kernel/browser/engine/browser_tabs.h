#ifndef ATRIX_BROWSER_TABS_H
#define ATRIX_BROWSER_TABS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// ATRIX Browser Tab Manager Subsystem
// ============================================================

#define ATRIX_MAX_TABS_LIMIT 16

typedef struct {
    uint32_t tab_id;
    char     title[64];
    char     url[128];
    bool     is_active;
    bool     is_loading;
} ATRIX_BrowserTab;

void             ATRIX_TabManager_Init(void);
ATRIX_BrowserTab* ATRIX_TabManager_CreateTab(const char* initial_url);
bool             ATRIX_TabManager_CloseTab(uint32_t tab_id);
ATRIX_BrowserTab* ATRIX_TabManager_GetActiveTab(void);
uint32_t         ATRIX_TabManager_GetTabCount(void);

#ifdef __cplusplus
}
#endif

#endif // ATRIX_BROWSER_TABS_H
