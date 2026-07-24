#ifndef ABE_WEB_OBSERVERS_H
#define ABE_WEB_OBSERVERS_H

#include "../../../sdk/include/abe/abe.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    OBSERVER_MUTATION = 0,
    OBSERVER_RESIZE,
    OBSERVER_INTERSECTION
} ABE_ObserverType;

typedef void (*ABE_ObserverCallback)(ABE_NodeHandle node, void* user_data);

typedef struct {
    ABE_ObserverHandle handle;
    ABE_ObserverType type;
    ABE_NodeHandle target_node;
    ABE_ObserverCallback cb;
    void* user_data;
    bool in_use;
} ABE_WebObserverInstance;

ABE_Error ABE_WebObservers_Init(void);
ABE_Error ABE_WebObservers_Shutdown(void);

ABE_Error ABE_WebObservers_CreateMutation(ABE_ObserverCallback cb, void* user_data, ABE_ObserverHandle* out_obs);
ABE_Error ABE_WebObservers_ObserveNode(ABE_ObserverHandle obs, ABE_NodeHandle node);
ABE_Error ABE_WebObservers_TriggerMutation(ABE_DocumentHandle doc, ABE_NodeHandle node);

#ifdef __cplusplus
}
#endif

#endif // ABE_WEB_OBSERVERS_H
