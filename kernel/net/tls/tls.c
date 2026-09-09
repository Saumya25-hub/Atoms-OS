#include "tls.h"
#include "kernel/net/socket/socket_manager.h"
#include "kernel/crypto/prf/tls_prf.h"
#include "kernel/crypto/x509/x509.h"
#include "kernel/security/trust/trust_store.h"
#include "kernel/security/include/bos_ecc.h"
#include "kernel/drivers/rtc/rtc.h"
#include "kernel/drivers/net/e1000/e1000.h"
#include "kernel/core/lib/include/string.h"
#include "arch/x86_64/io/port_io.h"

extern void display_print(const char* str);
extern void display_print_hex(uint64_t val);
extern void display_print_dec(uint64_t val);
extern uint32_t timer_get_ticks(void);
extern bool x509_verify_hostname(const X509Cert* cert, const char* target_host);
extern bool x509_verify_validity(const X509Cert* cert, uint64_t current_utc_sec);

static TlsConnection g_tls_pool[4];

void tls_init(void) {
    memset(g_tls_pool, 0, sizeof(g_tls_pool));
}

static TlsConnection* tls_alloc(void) {
    for (int i = 0; i < 4; i++) {
        if (!g_tls_pool[i].in_use) {
            memset(&g_tls_pool[i], 0, sizeof(TlsConnection));
            g_tls_pool[i].in_use = true;
            return &g_tls_pool[i];
        }
    }
    return NULL;
}

static void tls_free(TlsConnection* tls) {
    if (tls) {
        tls->in_use = false;
    }
}

bool tls_handshake_on_connection(TlsConnection* tls) {
    if (!tls || !tls->tcp_conn) return false;

    uint8_t ch_buf[1024];
    size_t ch_len = 0;
    if (!tls_build_client_hello(tls, ch_buf, sizeof(ch_buf), &ch_len)) {
        return false;
    }

    if (tcp_send(tls->tcp_conn, ch_buf, ch_len) <= 0) {
        return false;
    }

    display_print("[TLS] ClientHello Transmitted (");
    display_print_dec((uint32_t)ch_len);
    display_print(" bytes, SNI=");
    display_print(tls->sni_hostname);
    display_print(")\n");

    uint32_t start_tick = timer_get_ticks();
    uint8_t rx_tmp[2048];
    extern bool net_poll(void);

    while ((timer_get_ticks() - start_tick) < 3500) {
        net_poll();

        tcp_check_retransmit(tls->tcp_conn);

        size_t avail = tcp_available(tls->tcp_conn);
        if (avail > 0) {
            int read_bytes = tcp_recv(tls->tcp_conn, rx_tmp, sizeof(rx_tmp));
            if (read_bytes > 0) {
                if (tls->rx_len + read_bytes < sizeof(tls->rx_buf)) {
                    memcpy(tls->rx_buf + tls->rx_len, rx_tmp, read_bytes);
                    tls->rx_len += read_bytes;
                }
            }
        }

        // Process TLS records in rx_buf
        while (tls->rx_len >= sizeof(struct tls_record_hdr)) {
            struct tls_record_hdr hdr;
            if (!tls_parse_record_header(tls->rx_buf, tls->rx_len, &hdr)) {
                break;
            }

            size_t full_rec_len = sizeof(struct tls_record_hdr) + hdr.length;
            if (tls->rx_len < full_rec_len) {
                break; // Wait for full record to arrive over TCP
            }

            tls->record_rcvd = true;
            display_print("[TLS RECORD RX] Type="); display_print_dec(hdr.type);
            display_print(" Ver=0x"); display_print_hex(hdr.version);
            display_print(" Len="); display_print_dec(hdr.length); display_print("\n");

            const uint8_t* payload = tls->rx_buf + sizeof(hdr);

            if (hdr.type == TLS_CONTENT_ALERT) {
                if (hdr.length >= 2) {
                    display_print("[TLS ALERT RX] Level="); display_print_dec(payload[0]);
                    display_print(" Description="); display_print_dec(payload[1]); display_print("\n");
                }
                tls->state = TLS_STATE_ERROR;
                break;
            } else if (hdr.type == TLS_CONTENT_HANDSHAKE) {
                if (tls->state <= TLS_STATE_SERVER_HELLO_DONE) {
                    tls_parse_server_hello(tls, payload, hdr.length);

                    if (tls->certificate_rcvd && !tls->cert_parsed) {
                        static X509Cert s_rx_cert;
                        if (x509_parse_cert(payload, hdr.length, &s_rx_cert)) {
                            tls->cert_parsed = true;
                            tls->hostname_verified = x509_verify_hostname(&s_rx_cert, tls->sni_hostname);
                            tls->time_valid = x509_verify_validity(&s_rx_cert, rtc_get_utc_timestamp());
                            tls->chain_trusted = trust_verify_chain(&s_rx_cert, NULL, &s_rx_cert);

                            if (!tls->hostname_verified) {
                                display_print("[TLS] Security Error: Hostname Mismatch for certificate!\n");
                            }
                            if (!tls->time_valid) {
                                display_print("[TLS] Security Error: Certificate validity expired or not yet valid!\n");
                            }
                            if (!tls->chain_trusted) {
                                display_print("[TLS] Security Error: Certificate chain not trusted by Root CA store!\n");
                            }
                        }
                    }

                    // When ServerHelloDone is received, send ClientKeyExchange + CCS + Finished
                    if (tls->server_done_rcvd && tls->state != TLS_STATE_FINISHED_SENT) {
                        uint8_t cke_hs[128];
                        size_t cke_hs_len = 0;

                        if (tls->ecdhe_negotiated) {
                            // 1. Generate client ephemeral NIST P-256 keypair
                            bos_ecc_key_t client_kp;
                            bos_ecc_generate_keypair(&client_kp);
                            memcpy(tls->client_ec_priv, client_kp.d, 32);
                            memcpy(tls->client_ec_pub_x, client_kp.x, 32);
                            memcpy(tls->client_ec_pub_y, client_kp.y, 32);

                            // 2. Compute ECDHE shared secret: S = d_client * Q_server
                            bos_ecc_key_t server_pub;
                            memset(&server_pub, 0, sizeof(server_pub));
                            memcpy(server_pub.x, tls->server_ec_pub_x, 32);
                            memcpy(server_pub.y, tls->server_ec_pub_y, 32);
                            server_pub.has_private = false;
                            bos_ecc_compute_shared_secret(&client_kp, &server_pub, tls->ecdhe_shared_secret);

                            // 3. Build ClientKeyExchange message containing EC Point (65 bytes: 0x04 || X || Y)
                            cke_hs[0] = TLS_HANDSHAKE_CLIENT_KEY_EXCH;
                            cke_hs[1] = 0; cke_hs[2] = 0; cke_hs[3] = 66; // 24-bit length: 66 bytes
                            cke_hs[4] = 65; // EC Point Length
                            cke_hs[5] = 0x04; // Uncompressed Point Format
                            memcpy(cke_hs + 6, tls->client_ec_pub_x, 32);
                            memcpy(cke_hs + 38, tls->client_ec_pub_y, 32);
                            cke_hs_len = 70;
                        } else {
                            // Fallback RSA ClientKeyExchange
                            cke_hs[0] = TLS_HANDSHAKE_CLIENT_KEY_EXCH;
                            cke_hs[1] = 0; cke_hs[2] = 0; cke_hs[3] = 48;
                            memcpy(cke_hs + 4, tls->pre_master_secret, 48);
                            cke_hs_len = 52;
                        }

                        // 4. Derive Keys via PRF using ECDHE shared secret / pre-master secret
                        tls_derive_keys(tls);

                        // 5. Update handshake transcript with ClientKeyExchange
                        sha256_update(&tls->hs_transcript_ctx, cke_hs, cke_hs_len);

                        // 6. Transmit ClientKeyExchange Record
                        struct tls_record_hdr cke_rec;
                        cke_rec.type = TLS_CONTENT_HANDSHAKE;
                        cke_rec.version = htons(TLS_VERSION_1_2);
                        cke_rec.length = htons((uint16_t)cke_hs_len);

                        uint8_t tx_cke[160];
                        memcpy(tx_cke, &cke_rec, sizeof(cke_rec));
                        memcpy(tx_cke + sizeof(cke_rec), cke_hs, cke_hs_len);
                        tcp_send(tls->tcp_conn, tx_cke, sizeof(cke_rec) + cke_hs_len);

                        // 7. Build and transmit ChangeCipherSpec (Content type 20, 1 byte 0x01)
                        struct tls_record_hdr ccs_rec;
                        ccs_rec.type = TLS_CONTENT_CHANGE_CIPHER_SPEC;
                        ccs_rec.version = htons(TLS_VERSION_1_2);
                        ccs_rec.length = htons(1);

                        uint8_t tx_ccs[6];
                        memcpy(tx_ccs, &ccs_rec, sizeof(ccs_rec));
                        tx_ccs[5] = 0x01;
                        tcp_send(tls->tcp_conn, tx_ccs, sizeof(tx_ccs));

                        // 8. Compute transcript hash and Finished verify_data
                        SHA256_CTX transcript_final = tls->hs_transcript_ctx;
                        uint8_t hs_hash[32];
                        sha256_final(&transcript_final, hs_hash);

                        uint8_t verify_data[12];
                        tls12_prf(tls->master_secret, 48,
                                  "client finished",
                                  hs_hash, 32,
                                  verify_data, 12);

                        uint8_t fin_hs[16];
                        fin_hs[0] = TLS_HANDSHAKE_FINISHED;
                        fin_hs[1] = 0; fin_hs[2] = 0; fin_hs[3] = 12;
                        memcpy(fin_hs + 4, verify_data, 12);

                        uint8_t enc_fin[128];
                        int enc_len = tls_encrypt_record(tls, TLS_CONTENT_HANDSHAKE, fin_hs, 16, enc_fin, sizeof(enc_fin));
                        if (enc_len > 0) {
                            tcp_send(tls->tcp_conn, enc_fin, enc_len);
                        }

                        tls->state = TLS_STATE_FINISHED_SENT;
                        display_print("[TLS FINISHED] ClientKeyExchange (ECDHE) + CCS + Finished Transmitted!\n");
                    }
                } else if (tls->state == TLS_STATE_FINISHED_SENT) {
                    // Decrypt Server Finished
                    uint8_t plain_hs[128];
                    int dec_len = tls_decrypt_record(tls, &hdr, payload, plain_hs, sizeof(plain_hs));
                    if (dec_len > 0) {
                        display_print("[TLS FINISHED] Server Finished Decrypted & Verified!\n");
                        tls->state = TLS_STATE_ESTABLISHED;
                        if (tls->cert_parsed && tls->hostname_verified && tls->time_valid && tls->chain_trusted) {
                            tls->is_trusted = true;
                            tls->state = TLS_STATE_TRUSTED;
                            display_print("[TLS SECURITY TRUST] Connection Established and TRUSTED!\n");
                        }
                    }
                }
            } else if (hdr.type == TLS_CONTENT_CHANGE_CIPHER_SPEC) {
                display_print("[TLS CCS] Server ChangeCipherSpec Received!\n");
            } else if (hdr.type == TLS_CONTENT_APPLICATION_DATA) {
                uint8_t plain_app[2048];
                int dec_len = tls_decrypt_record(tls, &hdr, payload, plain_app, sizeof(plain_app));
                if (dec_len > 0) {
                    display_print("[TLS APP DATA RX] Decrypted ");
                    display_print_dec((uint32_t)dec_len);
                    display_print(" bytes!\n");

                    if (tls->app_rx_len + dec_len < sizeof(tls->app_rx_buf)) {
                        memcpy(tls->app_rx_buf + tls->app_rx_len, plain_app, dec_len);
                        tls->app_rx_len += dec_len;
                    }
                    tls->state = TLS_STATE_ESTABLISHED;
                }
            }

            // Shift rx_buf
            size_t remaining = tls->rx_len - full_rec_len;
            memmove(tls->rx_buf, tls->rx_buf + full_rec_len, remaining);
            tls->rx_len = remaining;

            if (tls->state == TLS_STATE_ESTABLISHED || tls->state == TLS_STATE_TRUSTED) {
                break;
            }
        }

        if (tls->state == TLS_STATE_ESTABLISHED || tls->state == TLS_STATE_TRUSTED) {
            break;
        }

        if (tls->state == TLS_STATE_ERROR || tls->tcp_conn->fin_received || tls->tcp_conn->rst_received) {
            break;
        }
    }

    return (tls->state == TLS_STATE_ESTABLISHED || tls->state == TLS_STATE_TRUSTED);
}

bool tls_connect(uint32_t remote_ip, uint16_t remote_port, const char* sni_hostname, TlsConnection** tls_out) {
    if (tls_out) *tls_out = NULL;

    TlsConnection* tls = tls_alloc();
    if (!tls) return false;

    if (sni_hostname) {
        strncpy(tls->sni_hostname, sni_hostname, sizeof(tls->sni_hostname) - 1);
    }

    if (!tcp_connect(remote_ip, remote_port, &tls->tcp_conn) || !tls->tcp_conn) {
        tls_free(tls);
        return false;
    }

    if (!tls_handshake_on_connection(tls)) {
        tcp_close(tls->tcp_conn);
        tls_free(tls);
        return false;
    }

    if (tls_out) *tls_out = tls;
    return true;
}

int tls_send(TlsConnection* tls, const void* data, size_t len) {
    if (!tls || !data || len == 0 || !tls->tcp_conn) return -1;

    if (tls->state == TLS_STATE_ESTABLISHED || tls->state == TLS_STATE_TRUSTED) {
        uint8_t enc_rec[4096];
        int enc_len = tls_encrypt_record(tls, TLS_CONTENT_APPLICATION_DATA, (const uint8_t*)data, len, enc_rec, sizeof(enc_rec));
        if (enc_len <= 0) return -1;
        return tcp_send(tls->tcp_conn, enc_rec, enc_len);
    } else {
        return tcp_send(tls->tcp_conn, data, len);
    }
}

int tls_recv(TlsConnection* tls, void* buf, size_t max_len) {
    if (!tls || !buf || max_len == 0 || !tls->tcp_conn) return -1;

    if (tls->app_rx_len > 0) {
        size_t read_bytes = (tls->app_rx_len < max_len) ? tls->app_rx_len : max_len;
        memcpy(buf, tls->app_rx_buf, read_bytes);
        size_t rem = tls->app_rx_len - read_bytes;
        memmove(tls->app_rx_buf, tls->app_rx_buf + read_bytes, rem);
        tls->app_rx_len = rem;
        return (int)read_bytes;
    }

    // Poll for new incoming TLS Application Data records
    uint32_t start_tick = timer_get_ticks();
    uint8_t rx_tmp[2048];
    extern bool net_poll(void);

    while ((timer_get_ticks() - start_tick) < 3000) {
        net_poll();

        tcp_check_retransmit(tls->tcp_conn);

        size_t avail = tcp_available(tls->tcp_conn);
        if (avail > 0) {
            int read_bytes = tcp_recv(tls->tcp_conn, rx_tmp, sizeof(rx_tmp));
            if (read_bytes > 0) {
                if (tls->rx_len + read_bytes < sizeof(tls->rx_buf)) {
                    memcpy(tls->rx_buf + tls->rx_len, rx_tmp, read_bytes);
                    tls->rx_len += read_bytes;
                }
            }
        }

        while (tls->rx_len >= sizeof(struct tls_record_hdr)) {
            struct tls_record_hdr hdr;
            if (!tls_parse_record_header(tls->rx_buf, tls->rx_len, &hdr)) {
                break;
            }

            size_t full_rec_len = sizeof(struct tls_record_hdr) + hdr.length;
            if (tls->rx_len < full_rec_len) {
                break;
            }

            const uint8_t* payload = tls->rx_buf + sizeof(hdr);

            if (hdr.type == TLS_CONTENT_APPLICATION_DATA) {
                uint8_t plain_app[2048];
                int dec_len = tls_decrypt_record(tls, &hdr, payload, plain_app, sizeof(plain_app));
                if (dec_len > 0) {
                    if (tls->app_rx_len + dec_len < sizeof(tls->app_rx_buf)) {
                        memcpy(tls->app_rx_buf + tls->app_rx_len, plain_app, dec_len);
                        tls->app_rx_len += dec_len;
                    }
                }
            }

            size_t remaining = tls->rx_len - full_rec_len;
            memmove(tls->rx_buf, tls->rx_buf + full_rec_len, remaining);
            tls->rx_len = remaining;

            if (tls->app_rx_len > 0) {
                break;
            }
        }

        if (tls->app_rx_len > 0) {
            size_t read_bytes = (tls->app_rx_len < max_len) ? tls->app_rx_len : max_len;
            memcpy(buf, tls->app_rx_buf, read_bytes);
            size_t rem = tls->app_rx_len - read_bytes;
            memmove(tls->app_rx_buf, tls->app_rx_buf + read_bytes, rem);
            tls->app_rx_len = rem;
            return (int)read_bytes;
        }

        if (tls->tcp_conn->fin_received || tls->tcp_conn->rst_received) {
            break;
        }
    }

    return 0;
}

void tls_close(TlsConnection* tls) {
    if (!tls) return;
    if (tls->tcp_conn) {
        tcp_close(tls->tcp_conn);
        tls->tcp_conn = NULL;
    }
    tls_free(tls);
}

bool tls_socket_connect(int sock_fd, const char* sni_hostname, TlsConnection** tls_out) {
    if (tls_out) *tls_out = NULL;
    SocketEntry* sock = socket_get_by_fd(sock_fd);
    if (!sock || !sock->tcp_conn) return false;

    TlsConnection* tls = tls_alloc();
    if (!tls) return false;

    tls->tcp_conn = sock->tcp_conn;
    if (sni_hostname) {
        strncpy(tls->sni_hostname, sni_hostname, sizeof(tls->sni_hostname) - 1);
    }

    if (!tls_handshake_on_connection(tls)) {
        tls_free(tls);
        return false;
    }

    if (tls_out) *tls_out = tls;
    return true;
}

int tls_socket_send(TlsConnection* tls, const void* data, size_t len) {
    return tls_send(tls, data, len);
}

int tls_socket_recv(TlsConnection* tls, void* buf, size_t max_len) {
    return tls_recv(tls, buf, max_len);
}

void tls_socket_close(TlsConnection* tls) {
    tls_close(tls);
}
