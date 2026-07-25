#ifndef BOS_HTML_H
#define BOS_HTML_H

#include "html_types.h"

// ============================================================================
// BOS OS — Phase 5A: HTML5 Parser & DOM Core Engine Public Facade Header
// ============================================================================

#ifdef __cplusplus
extern "C" {
#endif

// Subsystem Lifecycle
bos_html_status_t bos_html_init(void);

// Document Lifecycle
bos_html_status_t bos_document_create(const char* uri, bos_document_t** out_doc);
bos_html_status_t bos_document_destroy(bos_document_t* doc);

// Parser Core API
bos_html_status_t bos_html_parse(const char* html_input, bos_document_t** out_doc);

// Node Lifecycle & Factory
bos_html_status_t bos_node_create(bos_node_type_t type, const char* name, bos_node_t** out_node);
bos_html_status_t bos_node_destroy(bos_node_t* node);

// DOM Tree Mutation Operations
bos_html_status_t bos_append_child(bos_node_t* parent, bos_node_t* child);
bos_html_status_t bos_remove_child(bos_node_t* parent, bos_node_t* child);
bos_html_status_t bos_replace_child(bos_node_t* parent, bos_node_t* new_child, bos_node_t* old_child);
bos_html_status_t bos_insert_before(bos_node_t* parent, bos_node_t* new_node, bos_node_t* ref_node);
bos_html_status_t bos_clone_node(const bos_node_t* node, bool deep, bos_node_t** out_clone);

// Node Lookup Operations
bos_node_t* bos_find_element_by_id(const bos_document_t* doc, const char* id);
uint32_t bos_find_elements_by_tag(const bos_document_t* doc, const char* tag, bos_node_t** out_array, uint32_t max_count);
uint32_t bos_find_elements_by_class(const bos_document_t* doc, const char* class_name, bos_node_t** out_array, uint32_t max_count);

// Attribute Manipulation APIs
bos_html_status_t bos_set_attribute(bos_node_t* elem, const char* name, const char* value);
const char* bos_get_attribute(const bos_node_t* elem, const char* name);
bos_html_status_t bos_remove_attribute(bos_node_t* elem, const char* name);
bool bos_has_attribute(const bos_node_t* elem, const char* name);

// DOM Serialization
bos_html_status_t bos_dom_serialize(const bos_node_t* node, bool pretty, char* out_buf, uint32_t max_len);

// Diagnostics & Certification
void bos_html_get_diagnostics(html_diag_stats_t* out_stats);
void bos_html_run_certification_tests(void);

#ifdef __cplusplus
}
#endif

#endif // BOS_HTML_H
