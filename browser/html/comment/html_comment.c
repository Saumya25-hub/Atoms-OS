#include "html_comment.h"
#include "browser/html/node/html_node.h"
#include "kernel/core/lib/include/string.h"

bos_html_status_t html_comment_create(const char* comment_data, bos_node_t** out_comment) {
    if (!out_comment) return BOS_HTML_ERR_INVALID_PARAM;
    bos_node_t* node = NULL;
    bos_html_status_t status = html_node_create(BOS_NODE_COMMENT, "#comment", &node);
    if (status != BOS_HTML_OK) return status;

    if (comment_data) {
        strncpy(node->value, comment_data, sizeof(node->value) - 1);
    }
    *out_comment = node;
    return BOS_HTML_OK;
}
