#include "html_element.h"
#include "browser/html/node/html_node.h"
#include "browser/html/attributes/html_attribute.h"

bos_html_status_t html_element_create(const char* tag_name, bos_node_t** out_elem) {
    if (!tag_name || !out_elem) return BOS_HTML_ERR_INVALID_PARAM;
    return html_node_create(BOS_NODE_ELEMENT, tag_name, out_elem);
}

const char* html_element_get_id(const bos_node_t* elem) {
    return html_attr_get(elem, "id");
}

const char* html_element_get_class(const bos_node_t* elem) {
    return html_attr_get(elem, "class");
}
