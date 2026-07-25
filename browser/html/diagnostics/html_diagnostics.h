#ifndef HTML_DIAGNOSTICS_H
#define HTML_DIAGNOSTICS_H

#include "browser/html/include/html_types.h"

void html_diag_init(void);
void html_diag_on_node_alloc(void);
void html_diag_on_node_free(void);
void html_diag_on_attr_alloc(void);
void html_diag_on_attr_free(void);
void html_diag_on_token(void);
void html_diag_on_error_recovered(void);
void html_diag_record_time(uint32_t us);
void html_diag_get_stats(html_diag_stats_t* out_stats);

#endif // HTML_DIAGNOSTICS_H
