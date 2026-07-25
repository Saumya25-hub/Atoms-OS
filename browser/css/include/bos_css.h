#ifndef BOS_CSS_H
#define BOS_CSS_H

#include "browser/css/include/css_types.h"
#include "browser/html/include/html_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Subsystem Lifecycle
css_status_t bos_css_init(void);

// Parser & Stylesheet Lifecycles
css_status_t bos_css_parse(const char* css_input, css_stylesheet_t** out_sheet);
void bos_css_destroy_stylesheet(css_stylesheet_t* sheet);

// Selector & Style Computation APIs
bool bos_css_match_selector(const css_selector_t* selector, const bos_node_t* node);
css_status_t bos_css_compute_style(const bos_node_t* node, const css_stylesheet_t* sheet, css_computed_style_t* out_style);

// Diagnostic Dumps
void bos_css_dump_stylesheet(const css_stylesheet_t* sheet);

// Certification Test Suite
void bos_css_run_certification_tests(void);

#ifdef __cplusplus
}
#endif

#endif // BOS_CSS_H
