#include "html_fragment.h"
#include "browser/html/node/html_node.h"

bos_html_status_t html_fragment_create(bos_node_t** out_fragment) {
    if (!out_fragment) return BOS_HTML_ERR_INVALID_PARAM;
    return html_node_create(BOS_NODE_DOCUMENT_FRAGMENT, "#document-fragment", out_fragment);
}
