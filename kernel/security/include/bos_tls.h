/*
 * BOS OS — Phase 3: Production TLS & Security Engine
 * bos_tls.h — High-Performance TLS Engine Public API
 */

#ifndef BOS_TLS_H
#define BOS_TLS_H

#include "kernel/security/include/bos_security_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Establishes a TLS connection state object associated with target host and port.
 */
bos_sec_status_t bos_tls_connect(bos_tls_handle_t* handle, const char* hostname, uint16_t port);

/**
 * @brief Performs complete TLS 1.2 / 1.3 client handshake sequence.
 */
bos_sec_status_t bos_tls_handshake(bos_tls_handle_t handle);

/**
 * @brief Encrypts and transmits application data over TLS record layer.
 * @return Bytes transmitted or negative error code.
 */
int32_t bos_tls_send(bos_tls_handle_t handle, const void* data, size_t len);

/**
 * @brief Receives and decrypts application data from TLS record layer.
 * @return Bytes received into buffer or negative error code.
 */
int32_t bos_tls_receive(bos_tls_handle_t handle, void* buf, size_t max_len);

/**
 * @brief Sends closure alert and zeroizes connection context.
 */
bos_sec_status_t bos_tls_close(bos_tls_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif /* BOS_TLS_H */
