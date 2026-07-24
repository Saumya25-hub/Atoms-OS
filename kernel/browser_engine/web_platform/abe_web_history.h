#ifndef ABE_WEB_HISTORY_H
#define ABE_WEB_HISTORY_H

#include "../../../sdk/include/abe/abe.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ABE_MAX_HISTORY_ENTRIES 32

typedef struct {
    char url[ABE_MAX_URL_LEN];
    char state_data[256];
} ABE_HistoryEntry;

typedef struct {
    ABE_HistoryEntry stack[ABE_MAX_HISTORY_ENTRIES];
    uint32_t current_index;
    uint32_t count;
} ABE_WebHistory;

ABE_Error ABE_WebHistory_Init(void);
ABE_Error ABE_WebHistory_Shutdown(void);

ABE_Error ABE_WebHistory_PushState(const char* state_data, const char* title, const char* url);
ABE_Error ABE_WebHistory_ReplaceState(const char* state_data, const char* title, const char* url);
ABE_Error ABE_WebHistory_Back(void);
ABE_Error ABE_WebHistory_Forward(void);

#ifdef __cplusplus
}
#endif

#endif // ABE_WEB_HISTORY_H
