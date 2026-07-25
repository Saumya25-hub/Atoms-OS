#include "html_diagnostics.h"
#include "kernel/core/lib/include/string.h"

static html_diag_stats_t g_diag_stats;

void html_diag_init(void) {
    memset(&g_diag_stats, 0, sizeof(html_diag_stats_t));
}

void html_diag_on_node_alloc(void) {
    g_diag_stats.nodes_allocated++;
    g_diag_stats.active_nodes++;
}

void html_diag_on_node_free(void) {
    g_diag_stats.nodes_freed++;
    if (g_diag_stats.active_nodes > 0) {
        g_diag_stats.active_nodes--;
    }
}

void html_diag_on_attr_alloc(void) {
    g_diag_stats.attributes_allocated++;
}

void html_diag_on_attr_free(void) {
    // Stat tracker
}

void html_diag_on_token(void) {
    g_diag_stats.tokens_processed++;
}

void html_diag_on_error_recovered(void) {
    g_diag_stats.errors_recovered++;
}

void html_diag_record_time(uint32_t us) {
    g_diag_stats.parse_time_us += us;
}

void html_diag_get_stats(html_diag_stats_t* out_stats) {
    if (!out_stats) return;
    *out_stats = g_diag_stats;
}
