# ATOMS OS / ATRIX BROWSER — ARCHITECTURE PATCH PLAN
## Patch Specification: Network Error Isolation, BWE Surface Lifetime & Safe Pipeline Hardening

**Document ID:** ATRIX-PATCHPLAN-20260826-001  
**Target:** Elimination of Kernel #GP Fault on Network Failures & Error Page Rendering  
**Standard:** Rule 0 Phase Isolation Protocol (Investigate ➔ Plan ➔ Implement ➔ Build ➔ Runtime Verify ➔ Audit ➔ Certify)  
**Author:** ATOMS OS Architecture Authority  

---

## 1. Objectives & Scope of Changes

This patch plan defines the exact modifications required to:
1. Prevent kernel `#GP` faults on network/DNS/TLS error page generation and rendering.
2. Distinctly classify and display network error states (`DNS_FAILURE`, `TCP_FAILURE`, `TLS_HANDSHAKE_FAILURE`, `CERTIFICATE_FAILURE`, `HTTP_TIMEOUT`).
3. Harden the ABE DOM and RenderTree memory pool lifecycle by ensuring all node pointers are thoroughly cleared on destruction.
4. Correct text rendering in `atrix_paint_node_recursive` to eliminate unsafe dereferencing and double text emission.
5. Provide automatic word wrapping and viewport clamping for multi-line error descriptions.
6. Add deterministic regression test suite covering `BWE-DNS-ERROR-001` through `BWE-SURFACE-INVALIDATION-001`.

---

## 2. Detailed Modification Plan

### File 1: [`kernel/apps/atrix/atrix_browser.c`](file:///D:/Signatures_OS/kernel/apps/atrix/atrix_browser.c)
- **Functions:** `atrix_execute_browser_pipeline`, `atrix_paint_node_recursive`, `atrix_cleanup_active_page`, `atrix_render_callback`.
- **Modifications:**
  - Replace collapsed error string with a dedicated error generator function that inspects the exact `ABE_Error` return code (`ABE_ERR_NET_DNS_FAILED`, `ABE_ERR_NET_CONNECT_FAILED`, `ABE_ERR_NET_TLS_FAILED`, `ABE_ERR_NET_CERT_INVALID`, `ABE_ERR_NET_TIMEOUT`).
  - Generate distinct, well-structured error HTML documents with informative error badges.
  - In `atrix_paint_node_recursive`, ensure text is only painted when visiting `ABE_NODE_TEXT` nodes (or element nodes without text children), validating `dnode->in_use` and clipping coordinates before calling `BWE_DrawText`.
  - In `atrix_cleanup_active_page`, ensure atomic state transition so concurrent compositor runs never encounter partially destroyed trees.

### File 2: [`kernel/browser_engine/layout/abe_render_tree.c`](file:///D:/Signatures_OS/kernel/browser_engine/layout/abe_render_tree.c)
- **Function:** `FreeRenderNodeRecursive`.
- **Modifications:**
  - Explicitly clear `first_child`, `last_child`, `next_sibling`, `prev_sibling`, `parent`, and `dom_node_handle` to `NULL` / `ABE_INVALID_HANDLE` upon node deallocation.

### File 3: [`kernel/browser_engine/html/abe_dom_node.c`](file:///D:/Signatures_OS/kernel/browser_engine/html/abe_dom_node.c)
- **Function:** `ABE_DOM_DestroyNode`.
- **Modifications:**
  - Explicitly clear `first_child`, `last_child`, `next_sibling`, `prev_sibling`, `parent`, `attributes`, and `node_value` upon node deallocation.

### File 4: [`kernel/net/dns/dns.c`](file:///D:/Signatures_OS/kernel/net/dns/dns.c) & [`kernel/browser_engine/network/abe_net_dns.c`](file:///D:/Signatures_OS/kernel/browser_engine/network/abe_net_dns.c)
- **Functions:** `dns_resolve_ipv4`, `ABE_NetDNS_Resolve`.
- **Modifications:**
  - Fall back to standard DNS servers (`10.0.2.3` / `8.8.8.8`) if network interface has no DHCP assigned DNS server (`netif->dns_server == 0`).
  - Distinctly return `ABE_ERR_NET_DNS_FAILED` on unresolvable hosts.

### File 5: [`third_party/chromium_compatibility/tests/compatibility_test_suite.h`](file:///D:/Signatures_OS/third_party/chromium_compatibility/tests/compatibility_test_suite.h) & [`.cpp`](file:///D:/Signatures_OS/third_party/chromium_compatibility/tests/compatibility_test_suite.cpp)
- **Modifications:**
  - Add regression tests `BWE-DNS-ERROR-001`, `BWE-TCP-ERROR-001`, `BWE-TLS-ERROR-001`, `BWE-CERT-ERROR-001`, `BWE-SURFACE-LIFETIME-001`, and `BWE-SURFACE-INVALIDATION-001`.

---

## 3. Expected Results

1. When navigating to an unresolvable hostname (e.g. `https://www.google.com/search?q=youutube` with no gateway), DNS fails gracefully.
2. ATRIX Browser displays the dedicated DNS error page (`Server IP Address Not Found (DNS Failure) — DNS_PROBE_FINISHED_NXDOMAIN`).
3. Viewport and BWE surface render cleanly with zero kernel faults, zero `#GP`, and zero memory corruption.
4. Browser remains 100% interactive, allowing subsequent navigations to `about:compat`, `about:test`, `about:settings`, etc.
5. All regression tests pass deterministically.

---

## 4. Rollback Plan

If regressions occur, the affected files will be restored from git version control, and diagnostic serial traces will be analyzed.
