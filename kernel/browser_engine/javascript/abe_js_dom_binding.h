#ifndef ABE_JS_DOM_BINDING_H
#define ABE_JS_DOM_BINDING_H

#include "../../../sdk/include/abe/abe.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*ABE_JSEventListener)(ABE_NodeHandle target, const char* event_type, void* user_data);

typedef struct {
    ABE_NodeHandle target_node;
    char event_type[32];
    ABE_JSEventListener callback;
    void* user_data;
    bool in_use;
} ABE_JSEventBinding;

ABE_Error ABE_JSDOMBinding_Init(void);
ABE_Error ABE_JSDOMBinding_Shutdown(void);

ABE_Error ABE_JSDOM_AddEventListener(ABE_NodeHandle node, const char* event_type, ABE_JSEventListener cb, void* user_data);
ABE_Error ABE_JSDOM_DispatchEvent(ABE_DocumentHandle doc, ABE_NodeHandle node, const char* event_type);

ABE_Error ABE_JSDOM_SetInnerHTML(ABE_DocumentHandle doc, ABE_NodeHandle node, const char* html_str);

#ifdef __cplusplus
}
#endif

#endif // ABE_JS_DOM_BINDING_H
