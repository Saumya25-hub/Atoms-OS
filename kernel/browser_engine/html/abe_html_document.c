#include "abe_html_document.h"
#include "abe_html_element.h"
#include "../diagnostics/abe_diagnostics.h"
#include "kernel/core/lib/include/string.h"

static ABE_DocumentManager g_doc_mgr;
static uint32_t g_next_doc_id = 6000;
static bool g_doc_mgr_initialized = false;

static int StrCaseCmp(const char* s1, const char* s2) {
    if (!s1 || !s2) return 1;
    while (*s1 && *s2) {
        char c1 = (*s1 >= 'A' && *s1 <= 'Z') ? *s1 + 32 : *s1;
        char c2 = (*s2 >= 'A' && *s2 <= 'Z') ? *s2 + 32 : *s2;
        if (c1 != c2) return 1;
        s1++; s2++;
    }
    return (*s1 == '\0' && *s2 == '\0') ? 0 : 1;
}

ABE_Error ABE_HTMLDoc_Init(void) {
    memset(&g_doc_mgr, 0, sizeof(ABE_DocumentManager));
    g_doc_mgr_initialized = true;
    ABE_Log(ABE_LOG_INFO, "DOC", "ABE Document Lifecycle Manager initialized");
    return ABE_SUCCESS;
}

ABE_Error ABE_HTMLDoc_Shutdown(void) {
    if (!g_doc_mgr_initialized) return ABE_ERR_NOT_INITIALIZED;
    for (uint32_t i = 0; i < ABE_MAX_DOCUMENTS; i++) {
        if (g_doc_mgr.documents[i].in_use) {
            ABE_HTMLDoc_Destroy(g_doc_mgr.documents[i].handle);
        }
    }
    g_doc_mgr_initialized = false;
    ABE_Log(ABE_LOG_INFO, "DOC", "ABE Document Lifecycle Manager shut down cleanly");
    return ABE_SUCCESS;
}

ABE_DocumentStruct* ABE_HTMLDoc_Get(ABE_DocumentHandle handle) {
    if (!g_doc_mgr_initialized || handle == ABE_INVALID_HANDLE) return NULL;
    uint32_t slot = (handle >> 16) & 0xFFFF;
    if (slot >= ABE_MAX_DOCUMENTS) return NULL;
    if (g_doc_mgr.documents[slot].handle == handle && g_doc_mgr.documents[slot].in_use) {
        return &g_doc_mgr.documents[slot];
    }
    return NULL;
}

ABE_Error ABE_HTMLDoc_Create(ABE_DocumentHandle* out_doc) {
    if (!g_doc_mgr_initialized || !out_doc) return ABE_ERR_INVALID_PARAM;
    if (g_doc_mgr.active_count >= ABE_MAX_DOCUMENTS) return ABE_ERR_RESOURCE_EXHAUSTED;

    uint32_t slot = ABE_INVALID_HANDLE;
    for (uint32_t i = 0; i < ABE_MAX_DOCUMENTS; i++) {
        if (!g_doc_mgr.documents[i].in_use) {
            slot = i;
            break;
        }
    }

    if (slot == ABE_INVALID_HANDLE) return ABE_ERR_RESOURCE_EXHAUSTED;

    ABE_DocumentStruct* doc = &g_doc_mgr.documents[slot];
    memset(doc, 0, sizeof(ABE_DocumentStruct));
    doc->handle = (g_next_doc_id++) | (slot << 16);
    doc->ready_state = READYSTATE_LOADING;
    doc->in_use = true;

    // Create Root Document Node
    ABE_DOM_CreateNode(ABE_NODE_DOCUMENT, "#document", doc->handle, &doc->root_node);

    g_doc_mgr.active_count++;
    *out_doc = doc->handle;

    ABE_LogVal(ABE_LOG_INFO, "DOC", "Created new HTML Document, Handle: ", doc->handle);
    return ABE_SUCCESS;
}

ABE_Error ABE_HTMLDoc_Destroy(ABE_DocumentHandle handle) {
    if (!g_doc_mgr_initialized || handle == ABE_INVALID_HANDLE) return ABE_ERR_INVALID_PARAM;
    ABE_DocumentStruct* doc = ABE_HTMLDoc_Get(handle);
    if (!doc) return ABE_ERR_INVALID_PARAM;

    if (doc->root_node) {
        ABE_DOM_DestroyNode(doc->root_node);
        doc->root_node = NULL;
    }

    doc->in_use = false;
    if (g_doc_mgr.active_count > 0) g_doc_mgr.active_count--;

    ABE_LogVal(ABE_LOG_INFO, "DOC", "Destroyed HTML Document & freed DOM tree, Handle: ", handle);
    return ABE_SUCCESS;
}

static void RecursiveFindId(ABE_DOMNode* node, const char* id, ABE_DOMNode** out_node) {
    if (!node || *out_node) return;

    if (node->type == ABE_NODE_ELEMENT) {
        const char* node_id = ABE_HTMLAttr_Get(node, "id");
        if (node_id && strcmp(node_id, id) == 0) {
            *out_node = node;
            return;
        }
    }

    ABE_DOMNode* child = node->first_child;
    while (child) {
        RecursiveFindId(child, id, out_node);
        if (*out_node) return;
        child = child->next_sibling;
    }
}

ABE_Error ABE_HTMLDoc_FindById(ABE_DocumentHandle handle, const char* id, ABE_DOMNode** out_node) {
    if (!g_doc_mgr_initialized || handle == ABE_INVALID_HANDLE || !id || !out_node) return ABE_ERR_INVALID_PARAM;
    ABE_DocumentStruct* doc = ABE_HTMLDoc_Get(handle);
    if (!doc || !doc->root_node) return ABE_ERR_INVALID_PARAM;

    *out_node = NULL;
    RecursiveFindId(doc->root_node, id, out_node);
    return (*out_node != NULL) ? ABE_SUCCESS : ABE_ERR_DOM_NODE_NOT_FOUND;
}

static void RecursiveFindTag(ABE_DOMNode* node, const char* tag_name, ABE_NodeHandle* out_buf, uint32_t max_buf, uint32_t* out_count) {
    if (!node || *out_count >= max_buf) return;

    if (node->type == ABE_NODE_ELEMENT) {
        if (StrCaseCmp(tag_name, "*") == 0 || StrCaseCmp(node->tag_name, tag_name) == 0) {
            out_buf[(*out_count)++] = node->handle;
        }
    }

    ABE_DOMNode* child = node->first_child;
    while (child) {
        RecursiveFindTag(child, tag_name, out_buf, max_buf, out_count);
        if (*out_count >= max_buf) return;
        child = child->next_sibling;
    }
}

ABE_Error ABE_HTMLDoc_FindByTag(ABE_DocumentHandle handle, const char* tag_name, ABE_NodeHandle* out_buf, uint32_t max_buf, uint32_t* out_count) {
    if (!g_doc_mgr_initialized || handle == ABE_INVALID_HANDLE || !tag_name || !out_buf || !out_count) return ABE_ERR_INVALID_PARAM;
    ABE_DocumentStruct* doc = ABE_HTMLDoc_Get(handle);
    if (!doc || !doc->root_node) return ABE_ERR_INVALID_PARAM;

    *out_count = 0;
    RecursiveFindTag(doc->root_node, tag_name, out_buf, max_buf, out_count);
    return ABE_SUCCESS;
}
