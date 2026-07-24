#ifndef ABE_SESSION_H
#define ABE_SESSION_H

#include "../../../sdk/include/abe/abe.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t session_id;
    uint64_t start_time;
    uint32_t saved_windows_count;
    uint32_t saved_tabs_count;
    bool is_clean_exit;
} ABE_Session;

ABE_Error ABE_Session_Init(void);
ABE_Error ABE_Session_Shutdown(void);

ABE_Error ABE_Session_SaveState(void);
ABE_Error ABE_Session_RestoreState(void);

#ifdef __cplusplus
}
#endif

#endif // ABE_SESSION_H
