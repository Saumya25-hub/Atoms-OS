#ifndef KERNEL_X509_H
#define KERNEL_X509_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "kernel/crypto/rsa/rsa.h"

#define X509_MAX_SAN_NAMES 8
#define X509_NAME_MAX_LEN  128

typedef struct {
    uint32_t year;
    uint32_t month;
    uint32_t day;
    uint32_t hour;
    uint32_t minute;
    uint32_t second;
    uint64_t epoch_sec;
} X509Time;

typedef struct {
    char common_name[X509_NAME_MAX_LEN];
    char organization[X509_NAME_MAX_LEN];
    char country[16];
} X509Name;

typedef struct {
    uint8_t  serial[20];
    size_t   serial_len;

    X509Name issuer;
    X509Name subject;

    X509Time not_before;
    X509Time not_after;

    char     san_dns_names[X509_MAX_SAN_NAMES][X509_NAME_MAX_LEN];
    size_t   san_count;

    bool     is_ca;
    int      path_len_constraint;

    uint8_t  pubkey_algo[16];
    uint8_t  sig_algo[16];

    RsaPublicKey pubkey;

    const uint8_t* tbs_der;
    size_t         tbs_len;

    uint8_t  sig_bytes[512];
    size_t   sig_len;

    const uint8_t* der_raw;
    size_t         der_len;
} X509Cert;

bool x509_parse_cert(const uint8_t* der, size_t len, X509Cert* cert_out);

#endif // KERNEL_X509_H
