#include "../../../sdk/include/abe/abe.h"
#include "abe_dom_node.h"
#include "abe_html_element.h"
#include "abe_html_tokenizer.h"
#include "abe_html_parser.h"
#include "abe_html_text.h"
#include "abe_html_document.h"
#include "../diagnostics/abe_diagnostics.h"
#include "../network/abe_net_manager.h"
#include "kernel/core/lib/include/string.h"

static bool g_html_api_initialized = false;

ABE_Error ABE_HTMLInitialize(void) {
    if (g_html_api_initialized) return ABE_ERR_ALREADY_INITIALIZED;

    ABE_Error err;
    err = ABE_DOM_InitPool();
    if (err != ABE_SUCCESS) return err;

    err = ABE_HTMLElement_Init();
    if (err != ABE_SUCCESS) return err;

    err = ABE_HTMLText_Init();
    if (err != ABE_SUCCESS) return err;

    err = ABE_HTMLParser_Init();
    if (err != ABE_SUCCESS) return err;

    err = ABE_HTMLDoc_Init();
    if (err != ABE_SUCCESS) return err;

    g_html_api_initialized = true;
    ABE_Log(ABE_LOG_INFO, "HTML", "=========================================================");
    ABE_Log(ABE_LOG_INFO, "HTML", " ABE Phase 3 Production HTML5 Engine Initialized        ");
    ABE_Log(ABE_LOG_INFO, "HTML", "=========================================================");
    return ABE_SUCCESS;
}

ABE_Error ABE_HTMLShutdown(void) {
    if (!g_html_api_initialized) return ABE_ERR_NOT_INITIALIZED;

    ABE_HTMLDoc_Shutdown();
    ABE_HTMLParser_Shutdown();
    ABE_HTMLText_Shutdown();
    ABE_HTMLElement_Shutdown();
    ABE_DOM_ShutdownPool();

    g_html_api_initialized = false;
    ABE_Log(ABE_LOG_INFO, "HTML", "ABE Phase 3 Production HTML5 Engine shut down cleanly");
    return ABE_SUCCESS;
}

ABE_Error ABE_ParseHTML(const char* html_str, size_t len, ABE_DocumentHandle* out_doc) {
    if (!g_html_api_initialized || !html_str || len == 0 || !out_doc) return ABE_ERR_INVALID_PARAM;

    ABE_DocumentHandle doc_handle = ABE_INVALID_HANDLE;
    ABE_Error err = ABE_HTMLDoc_Create(&doc_handle);
    if (err != ABE_SUCCESS) return err;

    ABE_DocumentStruct* doc = ABE_HTMLDoc_Get(doc_handle);
    if (!doc) return ABE_ERR_INTERNAL_FAILURE;

    ABE_DOMNode* root_node = NULL;
    err = ABE_HTMLParser_ParseDocument(html_str, len, doc_handle, &root_node);
    if (err != ABE_SUCCESS) {
        ABE_HTMLDoc_Destroy(doc_handle);
        return err;
    }

    doc->root_node = root_node;
    doc->ready_state = READYSTATE_COMPLETE;
    *out_doc = doc_handle;
    return ABE_SUCCESS;
}

ABE_Error ABE_ParseHTMLStream(ABE_RequestHandle net_req, ABE_DocumentHandle* out_doc) {
    if (!g_html_api_initialized || net_req == ABE_INVALID_HANDLE || !out_doc) return ABE_ERR_INVALID_PARAM;

    ABE_HTTPResponse resp;
    ABE_Error err = ABE_ReadHTTPResponse(net_req, &resp);
    if (err != ABE_SUCCESS) return err;

    if (!resp.body_data || resp.body_len == 0) {
        ABE_FreeHTTPResponse(&resp);
        return ABE_ERR_NET_PARSE_FAILED;
    }

    err = ABE_ParseHTML((const char*)resp.body_data, resp.body_len, out_doc);
    ABE_FreeHTTPResponse(&resp);
    return err;
}

ABE_Error ABE_CreateDocument(ABE_DocumentHandle* out_doc) {
    return ABE_HTMLDoc_Create(out_doc);
}

ABE_Error ABE_DestroyDocument(ABE_DocumentHandle doc) {
    return ABE_HTMLDoc_Destroy(doc);
}

ABE_Error ABE_GetDocumentElement(ABE_DocumentHandle doc, ABE_NodeHandle* out_node) {
    if (!g_html_api_initialized || doc == ABE_INVALID_HANDLE || !out_node) return ABE_ERR_INVALID_PARAM;
    ABE_DocumentStruct* d = ABE_HTMLDoc_Get(doc);
    if (!d || !d->root_node) return ABE_ERR_INVALID_PARAM;

    // Find <html> child node
    ABE_DOMNode* child = d->root_node->first_child;
    while (child) {
        if (child->type == ABE_NODE_ELEMENT && strcmp(child->tag_name, "html") == 0) {
            *out_node = child->handle;
            return ABE_SUCCESS;
        }
        child = child->next_sibling;
    }
    return ABE_ERR_DOM_NODE_NOT_FOUND;
}

ABE_Error ABE_GetBody(ABE_DocumentHandle doc, ABE_NodeHandle* out_node) {
    if (!g_html_api_initialized || doc == ABE_INVALID_HANDLE || !out_node) return ABE_ERR_INVALID_PARAM;
    ABE_NodeHandle html_handle = ABE_INVALID_HANDLE;
    ABE_Error err = ABE_GetDocumentElement(doc, &html_handle);
    if (err != ABE_SUCCESS) return err;

    ABE_DOMNode* html_node = ABE_DOM_GetNodeByHandle(html_handle);
    if (!html_node) return ABE_ERR_DOM_NODE_NOT_FOUND;

    ABE_DOMNode* child = html_node->first_child;
    while (child) {
        if (child->type == ABE_NODE_ELEMENT && strcmp(child->tag_name, "body") == 0) {
            *out_node = child->handle;
            return ABE_SUCCESS;
        }
        child = child->next_sibling;
    }
    return ABE_ERR_DOM_NODE_NOT_FOUND;
}

ABE_Error ABE_FindElementById(ABE_DocumentHandle doc, const char* id, ABE_NodeHandle* out_node) {
    if (!g_html_api_initialized || doc == ABE_INVALID_HANDLE || !id || !out_node) return ABE_ERR_INVALID_PARAM;
    ABE_DOMNode* node = NULL;
    ABE_Error err = ABE_HTMLDoc_FindById(doc, id, &node);
    if (err != ABE_SUCCESS || !node) return ABE_ERR_DOM_NODE_NOT_FOUND;
    *out_node = node->handle;
    return ABE_SUCCESS;
}

ABE_Error ABE_FindElementsByTag(ABE_DocumentHandle doc, const char* tag_name, ABE_NodeHandle* out_buf, uint32_t max_buf, uint32_t* out_count) {
    if (!g_html_api_initialized || doc == ABE_INVALID_HANDLE || !tag_name || !out_buf || !out_count) return ABE_ERR_INVALID_PARAM;
    return ABE_HTMLDoc_FindByTag(doc, tag_name, out_buf, max_buf, out_count);
}

ABE_Error ABE_CreateElement(ABE_DocumentHandle doc, const char* tag_name, ABE_NodeHandle* out_node) {
    if (!g_html_api_initialized || doc == ABE_INVALID_HANDLE || !tag_name || !out_node) return ABE_ERR_INVALID_PARAM;
    ABE_DOMNode* node = NULL;
    ABE_Error err = ABE_DOM_CreateNode(ABE_NODE_ELEMENT, tag_name, doc, &node);
    if (err != ABE_SUCCESS || !node) return err;
    *out_node = node->handle;
    return ABE_SUCCESS;
}

ABE_Error ABE_CreateTextNode(ABE_DocumentHandle doc, const char* text, ABE_NodeHandle* out_node) {
    if (!g_html_api_initialized || doc == ABE_INVALID_HANDLE || !text || !out_node) return ABE_ERR_INVALID_PARAM;
    ABE_DOMNode* node = NULL;
    ABE_Error err = ABE_DOM_CreateNode(ABE_NODE_TEXT, text, doc, &node);
    if (err != ABE_SUCCESS || !node) return err;
    *out_node = node->handle;
    return ABE_SUCCESS;
}

ABE_Error ABE_AppendChild(ABE_NodeHandle parent, ABE_NodeHandle child) {
    if (!g_html_api_initialized || parent == ABE_INVALID_HANDLE || child == ABE_INVALID_HANDLE) return ABE_ERR_INVALID_PARAM;
    ABE_DOMNode* p = ABE_DOM_GetNodeByHandle(parent);
    ABE_DOMNode* c = ABE_DOM_GetNodeByHandle(child);
    if (!p || !c) return ABE_ERR_DOM_NODE_NOT_FOUND;
    return ABE_DOM_AppendChild(p, c);
}

ABE_Error ABE_InsertBefore(ABE_NodeHandle parent, ABE_NodeHandle new_node, ABE_NodeHandle ref_node) {
    if (!g_html_api_initialized || parent == ABE_INVALID_HANDLE || new_node == ABE_INVALID_HANDLE) return ABE_ERR_INVALID_PARAM;
    ABE_DOMNode* p = ABE_DOM_GetNodeByHandle(parent);
    ABE_DOMNode* n = ABE_DOM_GetNodeByHandle(new_node);
    ABE_DOMNode* r = (ref_node != ABE_INVALID_HANDLE) ? ABE_DOM_GetNodeByHandle(ref_node) : NULL;
    if (!p || !n) return ABE_ERR_DOM_NODE_NOT_FOUND;
    return ABE_DOM_InsertBefore(p, n, r);
}

ABE_Error ABE_RemoveChild(ABE_NodeHandle parent, ABE_NodeHandle child) {
    if (!g_html_api_initialized || parent == ABE_INVALID_HANDLE || child == ABE_INVALID_HANDLE) return ABE_ERR_INVALID_PARAM;
    ABE_DOMNode* p = ABE_DOM_GetNodeByHandle(parent);
    ABE_DOMNode* c = ABE_DOM_GetNodeByHandle(child);
    if (!p || !c) return ABE_ERR_DOM_NODE_NOT_FOUND;
    return ABE_DOM_RemoveChild(p, c);
}

ABE_Error ABE_GetNodeInfo(ABE_NodeHandle node, ABE_DOMNodeInfo* out_info) {
    if (!g_html_api_initialized || node == ABE_INVALID_HANDLE || !out_info) return ABE_ERR_INVALID_PARAM;
    ABE_DOMNode* n = ABE_DOM_GetNodeByHandle(node);
    if (!n) return ABE_ERR_DOM_NODE_NOT_FOUND;

    memset(out_info, 0, sizeof(ABE_DOMNodeInfo));
    out_info->handle = n->handle;
    out_info->type = n->type;
    strncpy(out_info->tag_name, n->tag_name, sizeof(out_info->tag_name) - 1);
    strncpy(out_info->node_value, n->node_value, sizeof(out_info->node_value) - 1);
    out_info->child_count = n->child_count;
    out_info->parent = n->parent ? n->parent->handle : ABE_INVALID_HANDLE;
    out_info->first_child = n->first_child ? n->first_child->handle : ABE_INVALID_HANDLE;
    out_info->last_child = n->last_child ? n->last_child->handle : ABE_INVALID_HANDLE;
    out_info->prev_sibling = n->prev_sibling ? n->prev_sibling->handle : ABE_INVALID_HANDLE;
    out_info->next_sibling = n->next_sibling ? n->next_sibling->handle : ABE_INVALID_HANDLE;
    out_info->attribute_count = n->attribute_count;
    memcpy(out_info->attributes, n->attributes, sizeof(n->attributes));

    return ABE_SUCCESS;
}

ABE_Error ABE_SetAttribute(ABE_NodeHandle node, const char* name, const char* value) {
    if (!g_html_api_initialized || node == ABE_INVALID_HANDLE || !name) return ABE_ERR_INVALID_PARAM;
    ABE_DOMNode* n = ABE_DOM_GetNodeByHandle(node);
    if (!n) return ABE_ERR_DOM_NODE_NOT_FOUND;
    return ABE_HTMLAttr_Set(n, name, value);
}

ABE_Error ABE_GetAttribute(ABE_NodeHandle node, const char* name, char* out_buf, size_t max_len) {
    if (!g_html_api_initialized || node == ABE_INVALID_HANDLE || !name || !out_buf || max_len == 0) return ABE_ERR_INVALID_PARAM;
    ABE_DOMNode* n = ABE_DOM_GetNodeByHandle(node);
    if (!n) return ABE_ERR_DOM_NODE_NOT_FOUND;
    const char* val = ABE_HTMLAttr_Get(n, name);
    if (!val) return ABE_ERR_DOM_NODE_NOT_FOUND;
    strncpy(out_buf, val, max_len - 1);
    out_buf[max_len - 1] = '\0';
    return ABE_SUCCESS;
}
