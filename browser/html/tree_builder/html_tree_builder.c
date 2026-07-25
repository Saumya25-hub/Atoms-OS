#include "html_tree_builder.h"
#include "browser/html/node/html_node.h"
#include "browser/html/element/html_element.h"
#include "browser/html/text/html_text.h"
#include "browser/html/comment/html_comment.h"
#include "browser/html/doctype/html_doctype.h"
#include "browser/html/attributes/html_attribute.h"
#include "browser/html/mutation/html_mutation.h"
#include "browser/html/diagnostics/html_diagnostics.h"
#include "kernel/core/lib/include/string.h"

static bool is_void_element(const char* name) {
    if (!name) return false;
    return (strcmp(name, "img") == 0 || strcmp(name, "br") == 0 ||
            strcmp(name, "hr") == 0 || strcmp(name, "input") == 0 ||
            strcmp(name, "meta") == 0 || strcmp(name, "link") == 0 ||
            strcmp(name, "base") == 0 || strcmp(name, "col") == 0 ||
            strcmp(name, "area") == 0 || strcmp(name, "embed") == 0 ||
            strcmp(name, "param") == 0 || strcmp(name, "source") == 0 ||
            strcmp(name, "track") == 0 || strcmp(name, "wbr") == 0);
}

static bool is_implicit_close(const char* current_tag, const char* new_tag) {
    if (!current_tag || !new_tag) return false;
    if (strcmp(current_tag, "p") == 0) {
        return (strcmp(new_tag, "p") == 0 || strcmp(new_tag, "div") == 0 ||
                strcmp(new_tag, "h1") == 0 || strcmp(new_tag, "h2") == 0 ||
                strcmp(new_tag, "h3") == 0 || strcmp(new_tag, "h4") == 0 ||
                strcmp(new_tag, "h5") == 0 || strcmp(new_tag, "h6") == 0 ||
                strcmp(new_tag, "ul") == 0 || strcmp(new_tag, "ol") == 0 ||
                strcmp(new_tag, "li") == 0 || strcmp(new_tag, "table") == 0);
    }
    if (strcmp(current_tag, "li") == 0 && strcmp(new_tag, "li") == 0) return true;
    if (strcmp(current_tag, "tr") == 0 && strcmp(new_tag, "tr") == 0) return true;
    if (strcmp(current_tag, "td") == 0 && (strcmp(new_tag, "td") == 0 || strcmp(new_tag, "tr") == 0)) return true;
    if (strcmp(current_tag, "option") == 0 && strcmp(new_tag, "option") == 0) return true;
    return false;
}

void html_tree_builder_init(html_tree_builder_t* tb, bos_document_t* doc) {
    if (!tb || !doc) return;
    tb->document = doc;
    tb->stack_top = -1;

    // Push Document Root onto stack
    if (doc->root_node) {
        tb->stack_top = 0;
        tb->stack[0] = doc->root_node;
    }
}

bos_node_t* html_tree_builder_current_node(const html_tree_builder_t* tb) {
    if (!tb || tb->stack_top < 0) return NULL;
    return tb->stack[tb->stack_top];
}

static void ensure_html_head_body(html_tree_builder_t* tb) {
    if (!tb || !tb->document) return;

    if (!tb->document->document_element) {
        bos_node_t* html_elem = NULL;
        if (html_element_create("html", &html_elem) == BOS_HTML_OK) {
            html_mutation_append_child(tb->document->root_node, html_elem);
            tb->document->document_element = html_elem;

            // Push html to stack
            if (tb->stack_top < MAX_OPEN_ELEMENTS - 1) {
                tb->stack[++tb->stack_top] = html_elem;
            }
        }
    }
}

bos_html_status_t html_tree_builder_process_token(html_tree_builder_t* tb, bos_html_token_t* token) {
    if (!tb || !token) return BOS_HTML_ERR_INVALID_PARAM;

    bos_node_t* current = html_tree_builder_current_node(tb);
    if (!current) return BOS_HTML_ERR_INVALID_PARAM;

    switch (token->type) {
        case BOS_TOKEN_DOCTYPE: {
            bos_node_t* doctype = NULL;
            if (html_doctype_create(token->name, "", "", &doctype) == BOS_HTML_OK) {
                html_mutation_append_child(tb->document->root_node, doctype);
            }
            break;
        }

        case BOS_TOKEN_COMMENT: {
            bos_node_t* comment = NULL;
            if (html_comment_create(token->data, &comment) == BOS_HTML_OK) {
                html_mutation_append_child(current, comment);
            }
            break;
        }

        case BOS_TOKEN_TEXT: {
            // Ignore leading whitespace if root or html
            if (current->type == BOS_NODE_DOCUMENT || (current->type == BOS_NODE_ELEMENT && strcmp(current->name, "html") == 0)) {
                bool all_space = true;
                for (int i = 0; token->data[i] != '\0'; i++) {
                    if (token->data[i] != ' ' && token->data[i] != '\t' && token->data[i] != '\n' && token->data[i] != '\r') {
                        all_space = false;
                        break;
                    }
                }
                if (all_space) break;
            }

            ensure_html_head_body(tb);
            current = html_tree_builder_current_node(tb);

            // Merge if last child is text node
            if (current && current->last_child && current->last_child->type == BOS_NODE_TEXT) {
                html_text_append(current->last_child, token->data);
            } else if (current) {
                bos_node_t* text_node = NULL;
                if (html_text_create(token->data, &text_node) == BOS_HTML_OK) {
                    html_mutation_append_child(current, text_node);
                }
            }
            break;
        }

        case BOS_TOKEN_START_TAG: {
            // Implicit tag closure checks
            if (current && current->type == BOS_NODE_ELEMENT && is_implicit_close(current->name, token->name)) {
                tb->stack_top--; // pop implicit closed tag
                current = html_tree_builder_current_node(tb);
                html_diag_on_error_recovered();
            }

            ensure_html_head_body(tb);
            current = html_tree_builder_current_node(tb);

            bos_node_t* elem = NULL;
            if (html_element_create(token->name, &elem) == BOS_HTML_OK) {
                // Deep-copy attributes from token (static pool) to heap-allocated elem attrs
                if (token->attributes) {
                    elem->attributes = html_attr_clone_all(token->attributes);
                }

                // Set special structural elements
                if (strcmp(token->name, "head") == 0) tb->document->head = elem;
                else if (strcmp(token->name, "body") == 0) tb->document->body = elem;

                html_mutation_append_child(current, elem);

                // Push to stack if not self closing or void tag
                if (!token->self_closing && !is_void_element(token->name)) {
                    if (tb->stack_top < MAX_OPEN_ELEMENTS - 1) {
                        tb->stack[++tb->stack_top] = elem;
                    }
                } else {
                    elem->self_closing = true;
                }
            }
            break;
        }

        case BOS_TOKEN_END_TAG: {
            // Pop stack until matching start tag found
            int match_idx = -1;
            for (int i = tb->stack_top; i >= 0; i--) {
                if (tb->stack[i]->type == BOS_NODE_ELEMENT && strcmp(tb->stack[i]->name, token->name) == 0) {
                    match_idx = i;
                    break;
                }
            }

            if (match_idx >= 0) {
                tb->stack_top = match_idx - 1; // pop up to matching element
            } else {
                html_diag_on_error_recovered(); // Unmatched closing tag, recover gracefully
            }
            break;
        }

        case BOS_TOKEN_EOF:
            break;
    }

    return BOS_HTML_OK;
}
