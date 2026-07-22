#include "x509.h"
#include "kernel/core/lib/include/string.h"

static const char* my_strchr(const char* s, int c) {
    if (!s) return NULL;
    while (*s) {
        if (*s == (char)c) return s;
        s++;
    }
    return (*s == (char)c) ? s : NULL;
}

static bool match_wildcard(const char* pattern, const char* hostname) {
    if (!pattern || !hostname) return false;

    if (pattern[0] == '*' && pattern[1] == '.') {
        const char* pattern_domain = pattern + 2;
        const char* dot = my_strchr(hostname, '.');
        if (!dot) return false;
        // Ensure left-most label matching only (no multi-level wildcard)
        const char* host_domain = dot + 1;
        return (strcmp(pattern_domain, host_domain) == 0);
    }

    return (strcmp(pattern, hostname) == 0);
}

bool x509_verify_hostname(const X509Cert* cert, const char* target_host) {
    if (!cert || !target_host || strlen(target_host) == 0) return false;

    // Check SAN entries first
    if (cert->san_count > 0) {
        for (size_t i = 0; i < cert->san_count; i++) {
            if (match_wildcard(cert->san_dns_names[i], target_host)) {
                return true;
            }
        }
        return false; // SAN present; do not fallback to CN
    }

    // Common Name fallback
    return match_wildcard(cert->subject.common_name, target_host);
}

bool x509_verify_validity(const X509Cert* cert, uint64_t current_utc_sec) {
    if (!cert) return false;
    if (current_utc_sec < cert->not_before.epoch_sec) return false; // Not yet valid
    if (current_utc_sec > cert->not_after.epoch_sec) return false;  // Expired
    return true;
}
