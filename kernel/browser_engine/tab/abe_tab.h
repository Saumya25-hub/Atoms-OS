#ifndef ABE_TAB_H
#define ABE_TAB_H

#include "../../../sdk/include/abe/abe.h"
#include "../navigation/abe_navigation.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ABE_MAX_GLOBAL_TABS 64

typedef enum {
    TAB_STATE_UNINIT = 0,
    TAB_STATE_LOADING = 1,
    TAB_STATE_ACTIVE = 2,
    TAB_STATE_SUSPENDED = 3,
    TAB_STATE_CRASHED = 4
} ABE_TabStateEnum;

typedef struct {
    ABE_TabHandle handle;
    ABE_WindowHandle owner_window;
    uint32_t tab_index;
    char title[ABE_MAX_TITLE_LEN];
    char current_url[ABE_MAX_URL_LEN];
    ABE_TabStateEnum state;
    bool is_active_in_window;
    bool is_pinned;
    uint32_t load_progress;
    ABE_NavigationHistory* history;
    uint64_t last_active_timestamp;
    void* dom_tree_context;
} ABE_TabNode;

typedef struct {
    ABE_TabNode tabs[ABE_MAX_GLOBAL_TABS];
    uint32_t total_tabs_count;
} ABE_TabEngineManager;

ABE_Error ABE_TabEngine_Init(void);
ABE_Error ABE_TabEngine_Shutdown(void);

ABE_Error ABE_Tab_Create(ABE_WindowHandle window, const char* initial_url, ABE_TabHandle* out_handle);
ABE_Error ABE_Tab_Close(ABE_WindowHandle window, ABE_TabHandle handle);
ABE_Error ABE_Tab_Select(ABE_WindowHandle window, ABE_TabHandle handle);
ABE_TabNode* ABE_Tab_Get(ABE_TabHandle handle);

ABE_Error ABE_Tab_LoadURL(ABE_TabHandle handle, const char* url);
ABE_Error ABE_Tab_Reload(ABE_TabHandle handle);
ABE_Error ABE_Tab_Stop(ABE_TabHandle handle);
ABE_Error ABE_Tab_GoBack(ABE_TabHandle handle);
ABE_Error ABE_Tab_GoForward(ABE_TabHandle handle);

#ifdef __cplusplus
}
#endif

#endif // ABE_TAB_H
