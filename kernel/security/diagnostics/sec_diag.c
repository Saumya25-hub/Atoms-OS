/*
 * BOS OS — Phase 3: Production TLS & Security Engine
 * sec_diag.c — Diagnostics & Telemetry Engine Implementation
 */

#include "kernel/security/diagnostics/sec_diag.h"
#include "kernel/core/lib/include/string.h"

extern void display_print(const char* str);
extern void display_print_dec(uint64_t val);

static sec_diag_metrics_t g_diag_metrics;

void sec_diag_init(void) {
    sec_diag_reset();
}

void sec_diag_reset(void) {
    memset(&g_diag_metrics, 0, sizeof(g_diag_metrics));
}

void sec_diag_record_handshake(uint64_t duration_us, bool success) {
    g_diag_metrics.total_handshakes++;
    g_diag_metrics.total_handshake_us += duration_us;
    if (success) {
        g_diag_metrics.successful_handshakes++;
    } else {
        g_diag_metrics.failed_handshakes++;
    }
}

void sec_diag_record_crypto(size_t bytes, bool encrypt) {
    g_diag_metrics.crypto_ops_count++;
    if (encrypt) {
        g_diag_metrics.bytes_encrypted += bytes;
    } else {
        g_diag_metrics.bytes_decrypted += bytes;
    }
}

void sec_diag_get_metrics(sec_diag_metrics_t* metrics) {
    if (metrics) {
        memcpy(metrics, &g_diag_metrics, sizeof(sec_diag_metrics_t));
    }
}

void sec_diag_dump_report(void) {
    display_print("=== BOS Security Engine Diagnostics Report ===\n");
    display_print("Total TLS Handshakes: ");
    display_print_dec(g_diag_metrics.total_handshakes);
    display_print("\nSuccessful Handshakes: ");
    display_print_dec(g_diag_metrics.successful_handshakes);
    display_print("\nSession Reuses: ");
    display_print_dec(g_diag_metrics.session_reuses);
    display_print("\nCrypto Operations: ");
    display_print_dec(g_diag_metrics.crypto_ops_count);
    display_print("\nBytes Encrypted: ");
    display_print_dec(g_diag_metrics.bytes_encrypted);
    display_print("\nBytes Decrypted: ");
    display_print_dec(g_diag_metrics.bytes_decrypted);
    display_print("\n=============================================\n");
}
