#ifndef SIGNATURES_TLS_H
#define SIGNATURES_TLS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "kernel/net/tcp/tcp.h"
#include "kernel/net/ethernet/ethernet.h"
#include "kernel/crypto/sha256/sha256.h"
#include "kernel/crypto/aes/aes.h"

#define TLS_VERSION_1_2        0x0303

#define TLS_CONTENT_CHANGE_CIPHER_SPEC 20
#define TLS_CONTENT_ALERT              21
#define TLS_CONTENT_HANDSHAKE          22
#define TLS_CONTENT_APPLICATION_DATA   23

#define TLS_HANDSHAKE_CLIENT_HELLO     1
#define TLS_HANDSHAKE_SERVER_HELLO     2
#define TLS_HANDSHAKE_CERTIFICATE      11
#define TLS_HANDSHAKE_SERVER_KEY_EXCH  12
#define TLS_HANDSHAKE_CERTIFICATE_REQ  13
#define TLS_HANDSHAKE_SERVER_HELLO_DONE 14
#define TLS_HANDSHAKE_CLIENT_KEY_EXCH  16
#define TLS_HANDSHAKE_FINISHED         20

#define TLS_MAX_RECORD_LEN             16384

typedef enum {
    TLS_STATE_IDLE = 0,
    TLS_STATE_CLIENT_HELLO_SENT,
    TLS_STATE_SERVER_HELLO_RECEIVED,
    TLS_STATE_CERTIFICATE_RECEIVED,
    TLS_STATE_SERVER_KEY_EXCH_RECEIVED,
    TLS_STATE_SERVER_HELLO_DONE,
    TLS_STATE_CLIENT_KEY_EXCH_SENT,
    TLS_STATE_CHANGE_CIPHER_SPEC_SENT,
    TLS_STATE_FINISHED_SENT,
    TLS_STATE_ESTABLISHED,
    TLS_STATE_TRUSTED,
    TLS_STATE_ERROR,
    TLS_STATE_CLOSED
} TlsState;

// 5-byte packed standard TLS Record Header
struct tls_record_hdr {
    uint8_t  type;    // ContentType
    uint16_t version; // ProtocolVersion (Big Endian)
    uint16_t length;  // Payload Length (Big Endian)
} __attribute__((packed));

// 4-byte packed TLS Handshake Header
struct tls_handshake_hdr {
    uint8_t msg_type;  // HandshakeType
    uint8_t length[3]; // 24-bit payload length (Big Endian)
} __attribute__((packed));

typedef struct {
    TcpConnection* tcp_conn;
    TlsState       state;

    uint8_t  client_random[32];
    uint8_t  server_random[32];
    uint16_t cipher_suite;
    uint16_t tls_version;

    char     sni_hostname[128];
    bool     record_rcvd;
    bool     server_hello_rcvd;
    bool     certificate_rcvd;
    bool     server_done_rcvd;

    bool     cert_parsed;
    bool     hostname_verified;
    bool     time_valid;
    bool     chain_trusted;
    bool     is_trusted;

    // Handshake Transcript Hasher
    SHA256_CTX hs_transcript_ctx;

    // TLS 1.2 Cryptographic Key Schedule
    uint8_t  pre_master_secret[48];
    uint8_t  master_secret[48];
    uint8_t  client_write_mac_key[32];
    uint8_t  server_write_mac_key[32];
    uint8_t  client_write_key[32];
    uint8_t  server_write_key[32];
    uint8_t  client_write_iv[16];
    uint8_t  server_write_iv[16];

    AES_CTX  client_aes;
    AES_CTX  server_aes;

    uint64_t client_seq_num;
    uint64_t server_seq_num;

    // Buffer for TLS record stream reassembly
    uint8_t  rx_buf[16384];
    size_t   rx_len;

    // Plaintext application data buffer
    uint8_t  app_rx_buf[8192];
    size_t   app_rx_len;

    bool     in_use;
} TlsConnection;

void tls_init(void);
bool tls_build_client_hello(TlsConnection* tls, uint8_t* out_buf, size_t max_buf, size_t* out_len);
bool tls_parse_record_header(const uint8_t* buf, size_t len, struct tls_record_hdr* hdr_out);
bool tls_parse_server_hello(TlsConnection* tls, const uint8_t* payload, size_t len);

void tls_derive_keys(TlsConnection* tls);
int  tls_encrypt_record(TlsConnection* tls, uint8_t type, const uint8_t* plain, size_t plain_len, uint8_t* out_rec, size_t max_rec_len);
int  tls_decrypt_record(TlsConnection* tls, const struct tls_record_hdr* hdr, const uint8_t* cipher_payload, uint8_t* out_plain, size_t max_plain_len);

bool tls_connect(uint32_t remote_ip, uint16_t remote_port, const char* sni_hostname, TlsConnection** tls_out);
int  tls_send(TlsConnection* tls, const void* data, size_t len);
int  tls_recv(TlsConnection* tls, void* buf, size_t max_len);
void tls_close(TlsConnection* tls);

bool tls_socket_connect(int sock_fd, const char* sni_hostname, TlsConnection** tls_out);
int  tls_socket_send(TlsConnection* tls, const void* data, size_t len);
int  tls_socket_recv(TlsConnection* tls, void* buf, size_t max_len);
void tls_socket_close(TlsConnection* tls);

#endif // SIGNATURES_TLS_H
