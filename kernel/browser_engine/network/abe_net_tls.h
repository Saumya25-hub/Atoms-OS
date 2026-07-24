#ifndef ABE_NET_TLS_H
#define ABE_NET_TLS_H

#include "../../../sdk/include/abe/abe.h"
#include "kernel/net/tls/tls.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    TlsConnection* tls_conn;
    char sni_hostname[256];
    bool is_established;
    bool is_certificate_verified;
    uint32_t tls_version;
} ABE_TLSSession;

ABE_Error ABE_NetTLS_Init(void);
ABE_Error ABE_NetTLS_Shutdown(void);

ABE_Error ABE_NetTLS_ConnectSocket(int sock_fd, const char* hostname, ABE_TLSSession** out_tls);
ABE_Error ABE_NetTLS_Send(ABE_TLSSession* tls, const void* data, size_t len, size_t* out_sent);
ABE_Error ABE_NetTLS_Recv(ABE_TLSSession* tls, void* buf, size_t max_len, size_t* out_rcvd);
ABE_Error ABE_NetTLS_Close(ABE_TLSSession* tls);

#ifdef __cplusplus
}
#endif

#endif // ABE_NET_TLS_H
