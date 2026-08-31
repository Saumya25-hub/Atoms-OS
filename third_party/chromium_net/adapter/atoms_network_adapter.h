/*
 * ATOMS OS — Chromium Network Adapter Header
 * Bridges Chromium URLLoader to ATOMS kernel networking (DNS, TCP, TLS)
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 */

#ifndef THIRD_PARTY_CHROMIUM_NET_ADAPTER_ATOMS_NETWORK_ADAPTER_H_
#define THIRD_PARTY_CHROMIUM_NET_ADAPTER_ATOMS_NETWORK_ADAPTER_H_

#include "third_party/chromium_net/base/gurl.h"
#include "third_party/chromium_net/http/http_request_headers.h"
#include "third_party/chromium_net/url_request/url_loader.h"

namespace net {

/*
 * Dispatches a single HTTP/HTTPS request through the real ATOMS kernel
 * networking stack:
 *
 *   GURL
 *     ↓
 *   AtomsNetworkAdapter_Fetch
 *     ↓
 *   ABE_NetDNS_Resolve (real UDP port 53)
 *     ↓
 *   ABE_NetConn_Connect (real TCP 3-way handshake)
 *     ↓
 *   ABE_NetTLS_Handshake (real TLS 1.2 ECDHE) [if HTTPS]
 *     ↓
 *   ABE_NetHTTP_SerializeRequest (HTTP/1.1 wire format)
 *     ↓
 *   send over TCP / TLS channel
 *     ↓
 *   receive HTTP response bytes
 *     ↓
 *   parse into URLLoaderResult
 */
URLLoaderResult AtomsNetworkAdapter_Fetch(const GURL& url, const HttpRequestHeaders& headers);

} // namespace net

#endif // THIRD_PARTY_CHROMIUM_NET_ADAPTER_ATOMS_NETWORK_ADAPTER_H_
