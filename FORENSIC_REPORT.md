# ATOMS OS / ATRIX BROWSER — FORENSIC INVESTIGATION REPORT
## Root-Cause Analysis: Kernel #GP Fault on Network/DNS Error Navigation

**Document ID:** ATRIX-FORENSIC-20260826-001  
**Severity:** CRITICAL / HIGH PRIORITY  
**Subsystem:** ABE Browser Engine / ATRIX UI / BWE Surface & Invalidation Pipeline  
**Fault Vector:** `#GP` General Protection Fault (Vector 13, Error Code 0)  
**Fault RIP:** `0x000000000019D447` (CPL 0 Kernel Mode)  
**Author:** ATOMS OS Forensic Audit Team  

---

## 1. Executive Summary & Incident Description

During live user navigation in ATRIX Browser to an external HTTPS URL (`https://www.google.com/search?q=youutube`), DNS resolution failed on unconfigured network interface. Following DNS failure, the browser generated a fallback error HTML document and processed it through the ABE pipeline (Tokenization ➔ DOM ➔ CSS ➔ Computed Styles ➔ Render Tree ➔ Layout ➔ BWE Surface Invalidation). Immediately following submission and surface invalidation, the kernel encountered a `#GP` General Protection Fault (Error Code 0) at `RIP: 0x000000000019D447`.

---

## 2. Root Cause Forensic Tracing

### 2.1 Disassembly & Symbol Address Mapping (RIP `0x19D447`)
Cross-referencing the kernel symbol map (`build/kernel.map`):
- `0x19D3B1`: `isr_common_stub`
- `0x19D3D8`: `isr_common_stub.no_switch`
- `0x19D420`: `isr_common_stub.kernel_segments`
- `0x19D42C`: `isr_common_stub.restore_gprs`
- **`0x19D447`**: `iretq` instruction (2 bytes before `0x19D449` `isr_stub_table`)

In x86_64 architecture, `#GP(0)` executing on `iretq` indicates that the interrupt return frame on the stack (specifically `RIP`, `CS`, `RFLAGS`, `RSP`, or `SS`) popped by `iretq` contained an invalid/non-canonical value or mismatched selector privilege.

### 2.2 Forensic Chain of Causality
1. **Network Error Collapsing:**
   In [`kernel/apps/atrix/atrix_browser.c`](file:///D:/Signatures_OS/kernel/apps/atrix/atrix_browser.c#L615-L639), `ABE_OpenConnection()` returned `ABE_ERR_NET_DNS_FAILED` upon DNS lookup failure. The browser incorrectly collapsed DNS, TCP, and TLS errors into a single generic message `"HTTPS TLS Connection or Certificate Verification Failed!"`.

2. **Error HTML Pipeline Processing:**
   A 206-character error paragraph was generated in `html_input`. The ABE DOM and Layout engines constructed render nodes for `<p>` and its child text node.

3. **Double / Unsafe DOM Dereference in Viewport Paint:**
   In [`kernel/apps/atrix/atrix_browser.c`](file:///D:/Signatures_OS/kernel/apps/atrix/atrix_browser.c#L1178-L1185) (`atrix_paint_node_recursive`):
   - For element nodes, the code checked `dnode->first_child && dnode->first_child->type == ABE_NODE_TEXT` without validating `in_use` or handle validity, dereferencing raw pointers.
   - Because `BuildRenderTree` already creates a child render node for text nodes, `atrix_paint_node_recursive` attempted to draw the same text twice with unclipped horizontal offsets exceeding screen width ($x > 2000$).

4. **Stale Pointer Retention in Memory Pools:**
   In [`kernel/browser_engine/layout/abe_render_tree.c`](file:///D:/Signatures_OS/kernel/browser_engine/layout/abe_render_tree.c#L74-L87) (`FreeRenderNodeRecursive`) and [`kernel/browser_engine/html/abe_dom_node.c`](file:///D:/Signatures_OS/kernel/browser_engine/html/abe_dom_node.c#L70-L87) (`ABE_DOM_DestroyNode`), when nodes were destroyed during `atrix_cleanup_active_page()`, `node->in_use` was cleared, but `first_child`, `last_child`, `prev_sibling`, `next_sibling`, and `parent` pointers were **not** zeroed out. When the background compositor thread (`bcm_compositor_thread`) or interrupt context traversed the tree during invalidation, it encountered stale child pointers.

5. **Text Layout Overrun & Stack Boundary Damage:**
   In [`kernel/ui/bofont/text_layout.c`](file:///D:/Signatures_OS/kernel/ui/bofont/text_layout.c#L67-L114) (`BOTextLayout_RunEx`), unbounded lines without explicit word-wrapping caused coordinate runaway into negative/overflow calculations, triggering stack frame corruption during deep recursive painting under task context.

---

## 3. Files Involved

1. [`kernel/apps/atrix/atrix_browser.c`](file:///D:/Signatures_OS/kernel/apps/atrix/atrix_browser.c):
   - Distinct network error handling (`DNS_FAILURE`, `TCP_FAILURE`, `TLS_HANDSHAKE_FAILURE`, `CERTIFICATE_FAILURE`).
   - Safe render tree traversal in `atrix_paint_node_recursive`.
   - Concurrency protection on active page destruction and render submission.
2. [`kernel/browser_engine/layout/abe_render_tree.c`](file:///D:/Signatures_OS/kernel/browser_engine/layout/abe_render_tree.c):
   - Clear all tree pointers (`first_child`, `last_child`, `next_sibling`, `prev_sibling`, `parent`) on free.
3. [`kernel/browser_engine/html/abe_dom_node.c`](file:///D:/Signatures_OS/kernel/browser_engine/html/abe_dom_node.c):
   - Clear all node pointers on free.
4. [`kernel/net/dns/dns.c`](file:///D:/Signatures_OS/kernel/net/dns/dns.c) & [`kernel/browser_engine/network/abe_net_dns.c`](file:///D:/Signatures_OS/kernel/browser_engine/network/abe_net_dns.c):
   - DNS fallback to default recursive DNS servers when `netif->dns_server == 0`.
5. [`kernel/wm/bwe/renderer/bwe_paint.c`](file:///D:/Signatures_OS/kernel/wm/bwe/renderer/bwe_paint.c) & [`kernel/ui/bofont/text_layout.c`](file:///D:/Signatures_OS/kernel/ui/bofont/text_layout.c):
   - Strict word-wrapping and viewport clipping for text drawing.

---

## 4. Risk Analysis & Safety Assessment

- **Kernel Integrity:** Network and browser rendering operations must run safely without any possibility of corrupting kernel stack frames or triggering `#GP`.
- **System Stability:** Network failures (DNS timeout, TCP drop, bad certs) are expected normal runtime events and must cleanly render informative HTML error pages while keeping the browser and OS responsive.

---

## 5. Suspected Fix Strategy (NO CODE)

1. Introduce distinct error dispatching in `atrix_execute_browser_pipeline` mapping each `ABE_Error` to a specific, well-formatted error page (`about:dns-error`, `about:tcp-error`, `about:tls-error`, `about:cert-error`).
2. Zero all sibling, parent, and child pointers upon node destruction in DOM and RenderTree pools.
3. Eliminate double text drawing in `atrix_paint_node_recursive` and validate `in_use` and bounds before any paint invocation.
4. Enable automatic word-wrap clamping in text rendering so text never overflows viewport boundaries.
5. Provide automatic DNS server fallback in `dns_resolve_ipv4` if interface DNS is unconfigured.
6. Add regression tests `BWE-DNS-ERROR-001`, `BWE-TLS-ERROR-001`, `BWE-CERT-ERROR-001`, `BWE-TCP-ERROR-001`, `BWE-SURFACE-LIFETIME-001`, and `BWE-SURFACE-INVALIDATION-001`.
