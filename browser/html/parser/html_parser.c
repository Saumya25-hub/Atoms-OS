#include "html_parser.h"
#include "browser/html/tokenizer/html_tokenizer.h"
#include "browser/html/tree_builder/html_tree_builder.h"
#include "browser/html/document/html_document.h"
#include "browser/html/diagnostics/html_diagnostics.h"
#include "kernel/core/lib/include/string.h"


bos_html_status_t html_parser_parse_string(const char* html_input, bos_document_t** out_doc) {
    if (!out_doc) return BOS_HTML_ERR_INVALID_PARAM;
    *out_doc = NULL;

    html_diag_init();

    bos_document_t* doc = NULL;
    bos_html_status_t status = html_document_create("about:blank", &doc);
    if (status != BOS_HTML_OK) return status;

    html_tokenizer_t tok;
    html_tokenizer_init(&tok, html_input ? html_input : "");

    html_tree_builder_t tb;
    html_tree_builder_init(&tb, doc);

    bos_html_token_t token;
    memset(&token, 0, sizeof(token));
    while (1) {
        status = html_tokenizer_next(&tok, &token);
        if (status != BOS_HTML_OK) {
            html_diag_on_error_recovered();
            break;
        }

        html_tree_builder_process_token(&tb, &token);

        if (token.type == BOS_TOKEN_EOF) {
            html_token_clear(&token);
            break;
        }
        html_token_clear(&token);
    }

    *out_doc = doc;
    return BOS_HTML_OK;
}
