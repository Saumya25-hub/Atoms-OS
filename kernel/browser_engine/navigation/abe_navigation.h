#ifndef ABE_NAVIGATION_H
#define ABE_NAVIGATION_H

#include "../../../sdk/include/abe/abe.h"
#include "../url/abe_url.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ABE_MAX_NAV_HISTORY 64

typedef enum {
    NAV_STATE_IDLE = 0,
    NAV_STATE_CONNECTING = 1,
    NAV_STATE_LOADING = 2,
    NAV_STATE_PARSING = 3,
    NAV_STATE_COMPLETE = 4,
    NAV_STATE_ERROR = 5,
    NAV_STATE_STOPPED = 6
} ABE_NavigationState;

typedef struct {
    ABE_URL url;
    char title[ABE_MAX_TITLE_LEN];
    uint64_t timestamp;
    uint32_t scroll_offset_y;
} ABE_NavigationEntry;

typedef struct {
    ABE_NavigationEntry entries[ABE_MAX_NAV_HISTORY];
    uint32_t current_index;
    uint32_t total_count;
    ABE_NavigationState state;
    uint32_t load_progress; // 0..100
} ABE_NavigationHistory;

ABE_Error ABE_Navigation_Init(void);
ABE_Error ABE_Navigation_CreateHistory(ABE_NavigationHistory** out_history);
ABE_Error ABE_Navigation_FreeHistory(ABE_NavigationHistory* history);

ABE_Error ABE_Navigation_LoadURL(ABE_NavigationHistory* history, const char* raw_url);
ABE_Error ABE_Navigation_Reload(ABE_NavigationHistory* history);
ABE_Error ABE_Navigation_Stop(ABE_NavigationHistory* history);
ABE_Error ABE_Navigation_GoBack(ABE_NavigationHistory* history);
ABE_Error ABE_Navigation_GoForward(ABE_NavigationHistory* history);

bool      ABE_Navigation_CanGoBack(const ABE_NavigationHistory* history);
bool      ABE_Navigation_CanGoForward(const ABE_NavigationHistory* history);

const ABE_NavigationEntry* ABE_Navigation_GetCurrentEntry(const ABE_NavigationHistory* history);

#ifdef __cplusplus
}
#endif

#endif // ABE_NAVIGATION_H
