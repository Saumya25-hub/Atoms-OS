#ifndef ABE_HTML_DOCUMENT_H
#define ABE_HTML_DOCUMENT_H

#include "../../../sdk/include/abe/abe.h"
#include "abe_dom_node.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ABE_MAX_DOCUMENTS 16

typedef enum {
    READYSTATE_LOADING = 0,
    READYSTATE_INTERACTIVE = 1,
    READYSTATE_COMPLETE = 2
} ABE_ReadyStateEnum;

typedef struct {
    ABE_DocumentHandle handle;
    ABE_DOMNode* root_node;
    ABE_ReadyStateEnum ready_state;
    char title[ABE_MAX_TITLE_LEN];
    char url[ABE_MAX_URL_LEN];
    bool in_use;
} ABE_DocumentStruct;

typedef struct {
    ABE_DocumentStruct documents[ABE_MAX_DOCUMENTS];
    uint32_t active_count;
} ABE_DocumentManager;

ABE_Error ABE_HTMLDoc_Init(void);
ABE_Error ABE_HTMLDoc_Shutdown(void);

ABE_Error ABE_HTMLDoc_Create(ABE_DocumentHandle* out_doc);
ABE_Error ABE_HTMLDoc_Destroy(ABE_DocumentHandle handle);

ABE_Error ABE_HTMLDoc_FindById(ABE_DocumentHandle handle, const char* id, ABE_DOMNode** out_node);
ABE_Error ABE_HTMLDoc_FindByTag(ABE_DocumentHandle handle, const char* tag_name, ABE_NodeHandle* out_buf, uint32_t max_buf, uint32_t* out_count);

ABE_DocumentStruct* ABE_HTMLDoc_Get(ABE_DocumentHandle handle);

#ifdef __cplusplus
}
#endif

#endif // ABE_HTML_DOCUMENT_H
