#include "html_doctype.h"
#include "browser/html/node/html_node.h"
#include "kernel/core/lib/include/string.h"

bos_html_status_t html_doctype_create(const char* name, const char* public_id, const char* system_id, bos_node_t** out_doctype) {
    if (!out_doctype) return BOS_HTML_ERR_INVALID_PARAM;
    bos_node_t* node = NULL;
    bos_html_status_t status = html_node_create(BOS_NODE_DOCTYPE, name ? name : "html", &node);
    if (status != BOS_HTML_OK) return status;

    // Store doctype info in value
    strcpy(node->value, "PUBLIC \"");
    strcat(node->value, public_id ? public_id : "");
    strcat(node->value, "\" \"");
    strcat(node->value, system_id ? system_id : "");
    strcat(node->value, "\"");

    *out_doctype = node;
    return BOS_HTML_OK;
}
