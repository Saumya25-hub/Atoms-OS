#include "html_serializer.h"
#include "kernel/core/lib/include/string.h"

static void append_str(char* buf, uint32_t max_len, uint32_t* pos, const char* str) {
    if (!buf || !str || *pos >= max_len - 1) return;
    while (*str && *pos < max_len - 1) {
        buf[*pos] = *str;
        (*pos)++;
        str++;
    }
    buf[*pos] = '\0';
}

static void append_indent(char* buf, uint32_t max_len, uint32_t* pos, int depth) {
    for (int i = 0; i < depth; i++) {
        append_str(buf, max_len, pos, "  ");
    }
}

bos_html_status_t html_serialize_node(const bos_node_t* node, bool pretty, int depth, char* out_buf, uint32_t max_len, uint32_t* out_len) {
    if (!node || !out_buf || max_len == 0) return BOS_HTML_ERR_INVALID_PARAM;

    uint32_t pos = out_len ? *out_len : 0;

    switch (node->type) {
        case BOS_NODE_DOCUMENT: {
            bos_node_t* child = node->first_child;
            while (child) {
                html_serialize_node(child, pretty, depth, out_buf, max_len, &pos);
                child = child->next_sibling;
            }
            break;
        }

        case BOS_NODE_DOCTYPE: {
            if (pretty) append_indent(out_buf, max_len, &pos, depth);
            append_str(out_buf, max_len, &pos, "<!DOCTYPE ");
            append_str(out_buf, max_len, &pos, node->name);
            append_str(out_buf, max_len, &pos, ">");
            if (pretty) append_str(out_buf, max_len, &pos, "\n");
            break;
        }

        case BOS_NODE_TEXT: {
            append_str(out_buf, max_len, &pos, node->value);
            break;
        }

        case BOS_NODE_COMMENT: {
            if (pretty) append_indent(out_buf, max_len, &pos, depth);
            append_str(out_buf, max_len, &pos, "<!--");
            append_str(out_buf, max_len, &pos, node->value);
            append_str(out_buf, max_len, &pos, "-->");
            if (pretty) append_str(out_buf, max_len, &pos, "\n");
            break;
        }

        case BOS_NODE_ELEMENT: {
            if (pretty && depth > 0) append_indent(out_buf, max_len, &pos, depth);
            append_str(out_buf, max_len, &pos, "<");
            append_str(out_buf, max_len, &pos, node->name);

            // Serialize attributes
            bos_attr_t* attr = node->attributes;
            while (attr) {
                append_str(out_buf, max_len, &pos, " ");
                append_str(out_buf, max_len, &pos, attr->name);
                if (attr->value[0] != '\0') {
                    append_str(out_buf, max_len, &pos, "=\"");
                    append_str(out_buf, max_len, &pos, attr->value);
                    append_str(out_buf, max_len, &pos, "\"");
                }
                attr = attr->next;
            }

            if (node->self_closing) {
                append_str(out_buf, max_len, &pos, " />");
                if (pretty) append_str(out_buf, max_len, &pos, "\n");
            } else {
                append_str(out_buf, max_len, &pos, ">");

                bool has_element_children = false;
                bos_node_t* child = node->first_child;
                while (child) {
                    if (child->type == BOS_NODE_ELEMENT) has_element_children = true;
                    child = child->next_sibling;
                }

                if (pretty && has_element_children) append_str(out_buf, max_len, &pos, "\n");

                child = node->first_child;
                while (child) {
                    html_serialize_node(child, pretty, depth + 1, out_buf, max_len, &pos);
                    child = child->next_sibling;
                }

                if (pretty && has_element_children) append_indent(out_buf, max_len, &pos, depth);
                append_str(out_buf, max_len, &pos, "</");
                append_str(out_buf, max_len, &pos, node->name);
                append_str(out_buf, max_len, &pos, ">");
                if (pretty) append_str(out_buf, max_len, &pos, "\n");
            }
            break;
        }

        default:
            break;
    }

    if (out_len) *out_len = pos;
    return BOS_HTML_OK;
}
