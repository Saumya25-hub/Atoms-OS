#include "tls.h"
#include "kernel/core/lib/include/string.h"
#include "arch/x86_64/io/port_io.h"

static uint32_t g_tls_entropy_seed = 0x87654321;

static void generate_client_random(uint8_t random_out[32]) {
    uint32_t ts = 0x669E1200; 
    random_out[0] = (ts >> 24) & 0xFF;
    random_out[1] = (ts >> 16) & 0xFF;
    random_out[2] = (ts >> 8) & 0xFF;
    random_out[3] = ts & 0xFF;

    for (int i = 4; i < 32; i++) {
        g_tls_entropy_seed = g_tls_entropy_seed * 1103515245 + 12345;
        random_out[i] = (uint8_t)((g_tls_entropy_seed >> 16) & 0xFF);
    }
}

bool tls_build_client_hello(TlsConnection* tls, uint8_t* out_buf, size_t max_buf, size_t* out_len) {
    if (!tls || !out_buf || !out_len || max_buf < 512) {
        return false;
    }

    generate_client_random(tls->client_random);

    uint8_t hs_buf[1024];
    memset(hs_buf, 0, sizeof(hs_buf));
    size_t pos = 0;

    // Handshake Header (Type 1 = ClientHello, 3 bytes len placeholder)
    hs_buf[0] = TLS_HANDSHAKE_CLIENT_HELLO;
    pos = 4; // Skip header

    // Client Version (TLS 1.2 = 0x0303)
    hs_buf[pos++] = 0x03;
    hs_buf[pos++] = 0x03;

    // Client Random (32 bytes)
    memcpy(hs_buf + pos, tls->client_random, 32);
    pos += 32;

    // Session ID Length (0)
    hs_buf[pos++] = 0x00;

    // Cipher Suites (Length = 14 bytes = 7 suites)
    // 0xC02F: TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256
    // 0xC030: TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384
    // 0xCCA8: TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256
    // 0x009C: TLS_RSA_WITH_AES_128_GCM_SHA256
    // 0x002F: TLS_RSA_WITH_AES_128_CBC_SHA
    // 0x0035: TLS_RSA_WITH_AES_256_CBC_SHA
    // 0x1301: TLS_AES_128_GCM_SHA256
    static const uint8_t cipher_suites[] = {
        0x00, 0x0E,
        0x13, 0x01,
        0xC0, 0x2F,
        0xC0, 0x30,
        0xCC, 0xA8,
        0x00, 0x9C,
        0x00, 0x2F,
        0x00, 0x35
    };
    memcpy(hs_buf + pos, cipher_suites, sizeof(cipher_suites));
    pos += sizeof(cipher_suites);

    // Compression Methods (Length = 1 byte: 0x00 null)
    hs_buf[pos++] = 0x01;
    hs_buf[pos++] = 0x00;

    // Extensions Start
    size_t ext_len_offset = pos;
    pos += 2; // Extension vector length placeholder

    // Extension 1: Server Name Indication (SNI = 0x0000)
    if (strlen(tls->sni_hostname) > 0) {
        size_t sni_name_len = strlen(tls->sni_hostname);
        hs_buf[pos++] = 0x00; hs_buf[pos++] = 0x00; // Type SNI
        uint16_t sni_ext_len = (uint16_t)(sni_name_len + 5);
        hs_buf[pos++] = (sni_ext_len >> 8) & 0xFF;
        hs_buf[pos++] = sni_ext_len & 0xFF;

        uint16_t sni_list_len = (uint16_t)(sni_name_len + 3);
        hs_buf[pos++] = (sni_list_len >> 8) & 0xFF;
        hs_buf[pos++] = sni_list_len & 0xFF;

        hs_buf[pos++] = 0x00; // NameType = host_name
        hs_buf[pos++] = (sni_name_len >> 8) & 0xFF;
        hs_buf[pos++] = sni_name_len & 0xFF;
        memcpy(hs_buf + pos, tls->sni_hostname, sni_name_len);
        pos += sni_name_len;
    }

    // Extension 2: Supported Groups (0x000A: secp256r1, x25519)
    hs_buf[pos++] = 0x00; hs_buf[pos++] = 0x0A;
    hs_buf[pos++] = 0x00; hs_buf[pos++] = 0x06; // Length 6
    hs_buf[pos++] = 0x00; hs_buf[pos++] = 0x04; // List length 4
    hs_buf[pos++] = 0x00; hs_buf[pos++] = 0x17; // secp256r1 (23)
    hs_buf[pos++] = 0x00; hs_buf[pos++] = 0x1D; // x25519 (29)

    // Extension 3: EC Point Formats (0x000B: uncompressed)
    hs_buf[pos++] = 0x00; hs_buf[pos++] = 0x0B;
    hs_buf[pos++] = 0x00; hs_buf[pos++] = 0x02; // Length 2
    hs_buf[pos++] = 0x01;                       // List length 1
    hs_buf[pos++] = 0x00;                       // uncompressed (0)

    // Extension 4: Signature Algorithms (0x000D: rsa_pkcs1_sha256, ecdsa_secp256r1_sha256)
    hs_buf[pos++] = 0x00; hs_buf[pos++] = 0x0D;
    hs_buf[pos++] = 0x00; hs_buf[pos++] = 0x06; // Length 6
    hs_buf[pos++] = 0x00; hs_buf[pos++] = 0x04; // List length 4
    hs_buf[pos++] = 0x04; hs_buf[pos++] = 0x01; // rsa_pkcs1_sha256 (0x0401)
    hs_buf[pos++] = 0x04; hs_buf[pos++] = 0x03; // ecdsa_secp256r1_sha256 (0x0403)

    // Extension 5: Supported Versions (0x002B: TLS 1.2)
    hs_buf[pos++] = 0x00; hs_buf[pos++] = 0x2B;
    hs_buf[pos++] = 0x00; hs_buf[pos++] = 0x03; // Length 3
    hs_buf[pos++] = 0x02;                       // List length 2
    hs_buf[pos++] = 0x03; hs_buf[pos++] = 0x03; // TLS 1.2

    // Set Extensions Length
    size_t total_ext_len = pos - (ext_len_offset + 2);
    hs_buf[ext_len_offset] = (total_ext_len >> 8) & 0xFF;
    hs_buf[ext_len_offset + 1] = total_ext_len & 0xFF;

    // Set Handshake Length
    size_t hs_payload_len = pos - 4;
    hs_buf[1] = (hs_payload_len >> 16) & 0xFF;
    hs_buf[2] = (hs_payload_len >> 8) & 0xFF;
    hs_buf[3] = hs_payload_len & 0xFF;

    // Wrap in TLS Record Header (Type 22 Handshake, Version 0x0301 / TLS 1.0 record layer per RFC 8446)
    struct tls_record_hdr rec;
    rec.type = TLS_CONTENT_HANDSHAKE;
    rec.version = htons(0x0301);
    rec.length = htons((uint16_t)pos);

    memcpy(out_buf, &rec, sizeof(rec));
    memcpy(out_buf + sizeof(rec), hs_buf, pos);

    *out_len = sizeof(rec) + pos;
    return true;
}

bool tls_parse_server_hello(TlsConnection* tls, const uint8_t* payload, size_t len) {
    if (!tls || !payload || len < 4) {
        return false;
    }

    uint8_t msg_type = payload[0];
    uint32_t msg_len = ((uint32_t)payload[1] << 16) | ((uint32_t)payload[2] << 8) | payload[3];

    if (4 + msg_len > len) {
        return false;
    }

    if (msg_type == TLS_HANDSHAKE_SERVER_HELLO) {
        if (msg_len < 38) return false;

        const uint8_t* p = payload + 4;
        tls->tls_version = ((uint16_t)p[0] << 8) | p[1];
        memcpy(tls->server_random, p + 2, 32);

        uint8_t sess_id_len = p[34];
        const uint8_t* p_cs = p + 35 + sess_id_len;

        tls->cipher_suite = ((uint16_t)p_cs[0] << 8) | p_cs[1];
        tls->server_hello_rcvd = true;
        tls->state = TLS_STATE_SERVER_HELLO_RECEIVED;
        return true;
    } else if (msg_type == TLS_HANDSHAKE_CERTIFICATE) {
        tls->certificate_rcvd = true;
        tls->state = TLS_STATE_CERTIFICATE_RECEIVED;
        return true;
    } else if (msg_type == TLS_HANDSHAKE_SERVER_HELLO_DONE) {
        tls->server_done_rcvd = true;
        tls->state = TLS_STATE_SERVER_HELLO_DONE;
        return true;
    }

    return false;
}
