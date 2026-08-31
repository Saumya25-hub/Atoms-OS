#include "tls.h"
#include "kernel/crypto/prf/tls_prf.h"
#include "kernel/crypto/gcm/gcm.h"
#include "kernel/crypto/random/crypto_rand.h"
#include "kernel/core/lib/include/string.h"

void tls_derive_keys(TlsConnection* tls) {
    if (!tls) return;

    size_t pms_len = 48;
    const uint8_t* pms_ptr = tls->pre_master_secret;

    if (tls->ecdhe_negotiated) {
        pms_ptr = tls->ecdhe_shared_secret;
        pms_len = 32;
    } else {
        // Fallback for RSA key exchange
        if (tls->pre_master_secret[0] == 0 && tls->pre_master_secret[1] == 0) {
            tls->pre_master_secret[0] = 0x03;
            tls->pre_master_secret[1] = 0x03;
            crypto_random_bytes(tls->pre_master_secret + 2, 46);
        }
    }

    // Seed = ClientRandom (32 bytes) || ServerRandom (32 bytes)
    uint8_t rand_seed[64];
    memcpy(rand_seed, tls->client_random, 32);
    memcpy(rand_seed + 32, tls->server_random, 32);

    // Master Secret = PRF(pre_master_secret, "master secret", ClientRandom + ServerRandom)
    tls12_prf(pms_ptr, pms_len,
              "master secret",
              rand_seed, 64,
              tls->master_secret, 48);

    // Expansion Seed = ServerRandom (32 bytes) || ClientRandom (32 bytes)
    uint8_t exp_seed[64];
    memcpy(exp_seed, tls->server_random, 32);
    memcpy(exp_seed + 32, tls->client_random, 32);

    // Key Block = PRF(master_secret, "key expansion", ServerRandom + ClientRandom)
    // For AES-128-GCM: 16-byte Client Key + 16-byte Server Key + 4-byte Client IV + 4-byte Server IV = 40 bytes
    uint8_t key_block[64];
    tls12_prf(tls->master_secret, 48,
              "key expansion",
              exp_seed, 64,
              key_block, 40);

    memcpy(tls->client_write_key, key_block, 16);
    memcpy(tls->server_write_key, key_block + 16, 16);
    memcpy(tls->client_write_iv, key_block + 32, 4);
    memcpy(tls->server_write_iv, key_block + 36, 4);

    aes_set_key(&tls->client_aes, tls->client_write_key, 16);
    aes_set_key(&tls->server_aes, tls->server_write_key, 16);

    tls->client_seq_num = 0;
    tls->server_seq_num = 0;
}

int tls_encrypt_record(TlsConnection* tls, uint8_t type, const uint8_t* plain, size_t plain_len, uint8_t* out_rec, size_t max_rec_len) {
    if (!tls || !plain || !out_rec || max_rec_len < plain_len + 5 + 8 + 16) {
        return -1;
    }

    uint8_t explicit_nonce[8];
    uint64_t seq = tls->client_seq_num++;
    for (int i = 0; i < 8; i++) {
        explicit_nonce[i] = (uint8_t)((seq >> ((7 - i) * 8)) & 0xFF);
    }

    uint8_t iv[12];
    memcpy(iv, tls->client_write_iv, 4);
    memcpy(iv + 4, explicit_nonce, 8);

    uint8_t aad[13];
    for (int i = 0; i < 8; i++) {
        aad[i] = (uint8_t)((seq >> ((7 - i) * 8)) & 0xFF);
    }
    aad[8] = type;
    aad[9] = 0x03;
    aad[10] = 0x03;
    aad[11] = (uint8_t)((plain_len >> 8) & 0xFF);
    aad[12] = (uint8_t)(plain_len & 0xFF);

    uint8_t cipher_payload[16384];
    uint8_t tag[16];

    if (gcm_encrypt(&tls->client_aes, iv, aad, 13, plain, plain_len, cipher_payload, tag) != 0) {
        return -1;
    }

    uint16_t total_payload_len = (uint16_t)(8 + plain_len + 16);

    struct tls_record_hdr rec;
    rec.type = type;
    rec.version = htons(TLS_VERSION_1_2);
    rec.length = htons(total_payload_len);

    memcpy(out_rec, &rec, sizeof(rec));
    memcpy(out_rec + 5, explicit_nonce, 8);
    memcpy(out_rec + 5 + 8, cipher_payload, plain_len);
    memcpy(out_rec + 5 + 8 + plain_len, tag, 16);

    return 5 + total_payload_len;
}

int tls_decrypt_record(TlsConnection* tls, const struct tls_record_hdr* hdr, const uint8_t* cipher_payload, uint8_t* out_plain, size_t max_plain_len) {
    if (!tls || !hdr || !cipher_payload || !out_plain) {
        return -1;
    }

    uint16_t rec_len = hdr->length;
    if (rec_len < 24) return -1; // Must have 8 bytes explicit nonce + 16 bytes tag

    size_t plain_len = rec_len - 24;
    if (plain_len > max_plain_len) return -1;

    const uint8_t* explicit_nonce = cipher_payload;
    const uint8_t* cipher_data = cipher_payload + 8;
    const uint8_t* tag = cipher_payload + 8 + plain_len;

    uint8_t iv[12];
    memcpy(iv, tls->server_write_iv, 4);
    memcpy(iv + 4, explicit_nonce, 8);

    uint64_t seq = tls->server_seq_num++;
    uint8_t aad[13];
    for (int i = 0; i < 8; i++) {
        aad[i] = (uint8_t)((seq >> ((7 - i) * 8)) & 0xFF);
    }
    aad[8] = hdr->type;
    aad[9] = (hdr->version >> 8) & 0xFF;
    aad[10] = hdr->version & 0xFF;
    aad[11] = (uint8_t)((plain_len >> 8) & 0xFF);
    aad[12] = (uint8_t)(plain_len & 0xFF);

    if (gcm_decrypt(&tls->server_aes, iv, aad, 13, cipher_data, plain_len, tag, out_plain) != 0) {
        return -1; // AEAD authentication failed
    }

    return (int)plain_len;
}
