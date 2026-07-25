#include "browser/html/include/bos_html.h"
#include "browser/html/document/html_document.h"
#include "browser/html/parser/html_parser.h"
#include "browser/html/node/html_node.h"
#include "browser/html/mutation/html_mutation.h"
#include "browser/html/attributes/html_attribute.h"
#include "browser/html/serialization/html_serializer.h"
#include "browser/html/diagnostics/html_diagnostics.h"
#include "browser/html/tests/html_tests.h"
#include "kernel/drivers/display/display.h"

bos_html_status_t bos_html_init(void) {
    html_diag_init();
    display_print("[HTML ENGINE]\n");
    display_print("Tokenizer Ready\n");
    display_print("Parser Ready\n");
    display_print("DOM Ready\n");
    display_print("Document Ready\n");
    display_print("Tree Builder Ready\n");
    display_print("Serializer Ready\n");
    display_print("Diagnostics Ready\n");
    return BOS_HTML_OK;
}

bos_html_status_t bos_document_create(const char* uri, bos_document_t** out_doc) {
    return html_document_create(uri, out_doc);
}

bos_html_status_t bos_document_destroy(bos_document_t* doc) {
    return html_document_destroy(doc);
}

bos_html_status_t bos_html_parse(const char* html_input, bos_document_t** out_doc) {
    return html_parser_parse_string(html_input, out_doc);
}

bos_html_status_t bos_node_create(bos_node_type_t type, const char* name, bos_node_t** out_node) {
    return html_node_create(type, name, out_node);
}

bos_html_status_t bos_node_destroy(bos_node_t* node) {
    return html_node_destroy(node);
}

bos_html_status_t bos_append_child(bos_node_t* parent, bos_node_t* child) {
    return html_mutation_append_child(parent, child);
}

bos_html_status_t bos_remove_child(bos_node_t* parent, bos_node_t* child) {
    return html_mutation_remove_child(parent, child);
}

bos_html_status_t bos_replace_child(bos_node_t* parent, bos_node_t* new_child, bos_node_t* old_child) {
    return html_mutation_replace_child(parent, new_child, old_child);
}

bos_html_status_t bos_insert_before(bos_node_t* parent, bos_node_t* new_node, bos_node_t* ref_node) {
    return html_mutation_insert_before(parent, new_node, ref_node);
}

bos_html_status_t bos_clone_node(const bos_node_t* node, bool deep, bos_node_t** out_clone) {
    return html_mutation_clone_node(node, deep, out_clone);
}

bos_node_t* bos_find_element_by_id(const bos_document_t* doc, const char* id) {
    return html_document_get_element_by_id(doc, id);
}

uint32_t bos_find_elements_by_tag(const bos_document_t* doc, const char* tag, bos_node_t** out_array, uint32_t max_count) {
    return html_document_get_elements_by_tag(doc, tag, out_array, max_count);
}

uint32_t bos_find_elements_by_class(const bos_document_t* doc, const char* class_name, bos_node_t** out_array, uint32_t max_count) {
    return html_document_get_elements_by_class(doc, class_name, out_array, max_count);
}

bos_html_status_t bos_set_attribute(bos_node_t* elem, const char* name, const char* value) {
    return html_attr_set(elem, name, value);
}

const char* bos_get_attribute(const bos_node_t* elem, const char* name) {
    return html_attr_get(elem, name);
}

bos_html_status_t bos_remove_attribute(bos_node_t* elem, const char* name) {
    return html_attr_remove(elem, name);
}

bool bos_has_attribute(const bos_node_t* elem, const char* name) {
    return html_attr_has(elem, name);
}

bos_html_status_t bos_dom_serialize(const bos_node_t* node, bool pretty, char* out_buf, uint32_t max_len) {
    uint32_t len = 0;
    out_buf[0] = '\0';
    return html_serialize_node(node, pretty, 0, out_buf, max_len, &len);
}

void bos_html_get_diagnostics(html_diag_stats_t* out_stats) {
    html_diag_get_stats(out_stats);
}

void bos_html_run_certification_tests(void) {
    html_run_all_certification_tests();
}
