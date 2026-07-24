#ifndef ABE_NET_MANAGER_H
#define ABE_NET_MANAGER_H

#include "../../../sdk/include/abe/abe.h"
#include "abe_net_dns.h"
#include "abe_net_conn.h"
#include "abe_net_http.h"
#include "abe_net_parser.h"
#include "abe_net_tls.h"
#include "abe_net_redirect.h"
#include "abe_net_compress.h"
#include "abe_net_download.h"
#include "../diagnostics/abe_diagnostics.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ABE_MAX_NETWORK_REQUESTS 64

typedef enum {
    REQ_STATE_IDLE = 0,
    REQ_STATE_PENDING = 1,
    REQ_STATE_CONNECTING = 2,
    REQ_STATE_SENDING = 3,
    REQ_STATE_RECEIVING = 4,
    REQ_STATE_COMPLETED = 5,
    REQ_STATE_FAILED = 6,
    REQ_STATE_CANCELLED = 7
} ABE_RequestStateEnum;

typedef struct {
    ABE_RequestHandle handle;
    ABE_ConnHandle conn_handle;
    ABE_HTTPRequest request;
    ABE_HTTPResponse response;
    ABE_HTTPResponseHeaderInfo header_info;
    ABE_RedirectHistory redirect_history;
    ABE_RequestStateEnum state;
    uint64_t start_time_us;
    uint64_t end_time_us;
} ABE_NetworkRequestNode;

typedef struct {
    ABE_NetworkRequestNode requests[ABE_MAX_NETWORK_REQUESTS];
    uint32_t active_request_count;
    bool is_initialized;
} ABE_NetworkManager;

ABE_Error ABE_NetManager_Init(void);
ABE_Error ABE_NetManager_Shutdown(void);

ABE_Error ABE_NetManager_SendRequest(ABE_ConnHandle conn, const ABE_HTTPRequest* req, ABE_RequestHandle* out_req);
ABE_Error ABE_NetManager_ReadResponse(ABE_RequestHandle req, ABE_HTTPResponse* out_resp);
ABE_Error ABE_NetManager_CancelRequest(ABE_RequestHandle req);

ABE_NetworkRequestNode* ABE_NetManager_GetNode(ABE_RequestHandle req);

#ifdef __cplusplus
}
#endif

#endif // ABE_NET_MANAGER_H
