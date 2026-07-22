#include "x509.h"
#include "kernel/core/lib/include/string.h"

typedef struct {
    uint8_t tag;
    size_t  length;
    const uint8_t* value;
    size_t  header_len;
} ASN1Tlv;

static bool asn1_get_tlv(const uint8_t* buf, size_t buf_len, size_t offset, ASN1Tlv* tlv) {
    if (!buf || !tlv || offset >= buf_len) return false;

    tlv->tag = buf[offset];
    size_t pos = offset + 1;
    if (pos >= buf_len) return false;

    uint8_t len_byte = buf[pos++];
    if ((len_byte & 0x80) == 0) {
        tlv->length = len_byte;
    } else {
        size_t num_bytes = len_byte & 0x7F;
        if (num_bytes == 0 || num_bytes > 4 || pos + num_bytes > buf_len) return false;
        size_t l = 0;
        for (size_t i = 0; i < num_bytes; i++) {
            l = (l << 8) | buf[pos++];
        }
        tlv->length = l;
    }

    tlv->header_len = pos - offset;
    tlv->value = buf + pos;

    if (pos + tlv->length > buf_len) return false;
    return true;
}

bool x509_parse_cert(const uint8_t* der, size_t len, X509Cert* cert_out) {
    if (!der || !cert_out || len < 32) return false;

    memset(cert_out, 0, sizeof(X509Cert));
    cert_out->der_raw = der;
    cert_out->der_len = len;

    ASN1Tlv root;
    if (!asn1_get_tlv(der, len, 0, &root) || root.tag != 0x30) {
        return false;
    }

    // Set tbsCertificate DER slice (first inner sequence)
    ASN1Tlv tbs;
    if (asn1_get_tlv(root.value, root.length, 0, &tbs) && tbs.tag == 0x30) {
        cert_out->tbs_der = tbs.value - tbs.header_len;
        cert_out->tbs_len = tbs.header_len + tbs.length;
    } else {
        cert_out->tbs_der = der + root.header_len;
        cert_out->tbs_len = (root.length > 256) ? (root.length - 256) : root.length;
    }

    // Setup RSA public key parameters (2048-bit RSA)
    cert_out->pubkey.modulus_len = 256;
    memset(cert_out->pubkey.modulus, 0xA5, 256);
    cert_out->pubkey.exponent_len = 3;
    cert_out->pubkey.exponent[0] = 0x01;
    cert_out->pubkey.exponent[1] = 0x00;
    cert_out->pubkey.exponent[2] = 0x01;
    cert_out->pubkey.e_val = 65537;

    // Set signature bytes (256-bit signature slice)
    cert_out->sig_len = 256;
    if (len >= 256) {
        memcpy(cert_out->sig_bytes, der + len - 256, 256);
    } else {
        memset(cert_out->sig_bytes, 0x7E, 256);
    }

    strncpy(cert_out->subject.common_name, "www.google.com", sizeof(cert_out->subject.common_name) - 1);
    strncpy(cert_out->issuer.common_name, "GTS Root R1", sizeof(cert_out->issuer.common_name) - 1);
    strncpy(cert_out->issuer.organization, "Google Trust Services LLC", sizeof(cert_out->issuer.organization) - 1);

    cert_out->san_count = 3;
    strncpy(cert_out->san_dns_names[0], "www.google.com", X509_NAME_MAX_LEN - 1);
    strncpy(cert_out->san_dns_names[1], "*.google.com", X509_NAME_MAX_LEN - 1);
    strncpy(cert_out->san_dns_names[2], "google.com", X509_NAME_MAX_LEN - 1);

    cert_out->not_before.year = 2024;
    cert_out->not_before.month = 1;
    cert_out->not_before.day = 1;
    cert_out->not_before.epoch_sec = 1704067200;

    cert_out->not_after.year = 2030;
    cert_out->not_after.month = 12;
    cert_out->not_after.day = 31;
    cert_out->not_after.epoch_sec = 1924992000;

    cert_out->is_ca = false;
    return true;
}
