#ifndef ABE_NET_CONN_H
#define ABE_NET_CONN_H

#include "../../../sdk/include/abe/abe.h"
#include "abe_net_tls.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ABE_CONN_POOL_CAPACITY 32
#define ABE_MAX_CONNS_PER_HOST 6

typedef enum {
    CONN_STATE_FREE = 0,
    CONN_STATE_CONNECTING = 1,
    CONN_STATE_ESTABLISHED = 2,
    CONN_STATE_REUSED = 3,
    CONN_STATE_CLOSING = 4,
    CONN_STATE_CLOSED = 5
} ABE_ConnStateEnum;

typedef struct {
    ABE_ConnHandle handle;
    char host[256];
    uint16_t port;
    uint32_t resolved_ip;
    bool is_tls;
    int socket_fd;
    ABE_TLSSession* tls_session;
    ABE_ConnStateEnum state;
    uint32_t request_count;
    uint64_t last_used_timestamp;
    bool is_keep_alive;
} ABE_ConnectionNode;

typedef struct {
    ABE_ConnectionNode connections[ABE_CONN_POOL_CAPACITY];
    uint32_t active_connection_count;
    uint32_t max_idle_ms;
} ABE_ConnectionPool;

ABE_Error ABE_NetConn_Init(void);
ABE_Error ABE_NetConn_Shutdown(void);

ABE_Error ABE_NetConn_Open(const char* host, uint16_t port, bool use_tls, ABE_ConnHandle* out_conn);
ABE_Error ABE_NetConn_Close(ABE_ConnHandle handle);
ABE_Error ABE_NetConn_Send(ABE_ConnHandle handle, const void* data, size_t len, size_t* out_sent);
ABE_Error ABE_NetConn_Recv(ABE_ConnHandle handle, void* buf, size_t max_len, size_t* out_rcvd);

ABE_ConnectionNode* ABE_NetConn_Get(ABE_ConnHandle handle);

#ifdef __cplusplus
}
#endif

#endif // ABE_NET_CONN_H
