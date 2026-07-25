#include "html_text.h"
#include "browser/html/node/html_node.h"
#include "kernel/core/lib/include/string.h"

bos_html_status_t html_text_create(const char* text_content, bos_node_t** out_text) {
    if (!out_text) return BOS_HTML_ERR_INVALID_PARAM;
    bos_node_t* node = NULL;
    bos_html_status_t status = html_node_create(BOS_NODE_TEXT, "#text", &node);
    if (status != BOS_HTML_OK) return status;

    if (text_content) {
        strncpy(node->value, text_content, sizeof(node->value) - 1);
    }
    *out_text = node;
    return BOS_HTML_OK;
}

bos_html_status_t html_text_append(bos_node_t* text_node, const char* data) {
    if (!text_node || !data || text_node->type != BOS_NODE_TEXT) return BOS_HTML_ERR_INVALID_PARAM;

    size_t curr_len = strlen(text_node->value);
    size_t i = 0;
    while (data[i] != '\0' && curr_len + i < sizeof(text_node->value) - 1) {
        text_node->value[curr_len + i] = data[i];
        i++;
    }
    text_node->value[curr_len + i] = '\0';
    return BOS_HTML_OK;
}

bos_html_status_t html_text_split(bos_node_t* text_node, uint32_t offset, bos_node_t** out_new_node) {
    if (!text_node || text_node->type != BOS_NODE_TEXT || !out_new_node) return BOS_HTML_ERR_INVALID_PARAM;

    size_t len = strlen(text_node->value);
    if (offset > len) return BOS_HTML_ERR_INVALID_PARAM;

    bos_node_t* new_node = NULL;
    bos_html_status_t status = html_text_create(&text_node->value[offset], &new_node);
    if (status != BOS_HTML_OK) return status;

    text_node->value[offset] = '\0';
    *out_new_node = new_node;
    return BOS_HTML_OK;
}
