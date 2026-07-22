#include "tls.h"
#include "kernel/core/lib/include/string.h"

bool tls_parse_record_header(const uint8_t* buf, size_t len, struct tls_record_hdr* hdr_out) {
    if (!buf || !hdr_out || len < sizeof(struct tls_record_hdr)) {
        return false;
    }

    const struct tls_record_hdr* raw = (const struct tls_record_hdr*)buf;
    hdr_out->type = raw->type;
    hdr_out->version = ntohs(raw->version);
    hdr_out->length = ntohs(raw->length);

    // Bounds checking on record payload length
    if (hdr_out->length > TLS_MAX_RECORD_LEN) {
        return false;
    }

    return true;
}
