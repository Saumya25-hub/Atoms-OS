# CHROMIUM SANDBOX & WEB SECURITY TO ATOMS OS MAPPING MATRIX

**Document ID:** ATRIX-PHASE15-MATRIX-001  
**Phase:** Phase 15 — Sandbox + Web Security  
**Date:** 2026-08-26  

---

## 1. Chromium Security to ATOMS OS Concept Mapping

| Chromium Security Concept | Upstream Chromium Location | ATOMS OS Implementation | Status |
|:---|:---|:---|:---:|
| **Renderer Sandbox (Linux seccomp-bpf / Windows Token)** | `sandbox/linux/` / `sandbox/win/` | `kernel/sandbox/` (Capability tokens & Syscall filter) | **CERTIFIED** |
| **W^X / NX Memory Protection** | `base/allocator/partition_allocator/` | `kernel/core/memory/vmm/` (`VMM_FLAG_NX`) | **CERTIFIED** |
| **Mojo IPC Message Validation** | `mojo/public/cpp/bindings/` | `mojo/public/cpp/system/message.cpp` | **CERTIFIED** |
| **SecurityOrigin (Same-Origin Policy)** | `services/network/public/cpp/` | `third_party/chromium_net/base/security_origin.cpp` | **CERTIFIED** |
| **CanonicalCookie Security (`HttpOnly`, `Secure`)** | `net/cookies/canonical_cookie.cc` | `third_party/chromium_net/cookies/canonical_cookie.cpp` | **CERTIFIED** |
| **Storage Origin Partitioning** | `components/services/storage/` | `third_party/chromium_storage/dom_storage/` | **CERTIFIED** |
| **Content Security Policy (CSP)** | `third_party/blink/renderer/core/frame/csp/` | `third_party/chromium_net/base/content_security_policy.cpp` | **CERTIFIED** |
| **X-Frame-Options (Clickjacking)** | `third_party/blink/renderer/core/frame/` | `third_party/chromium_net/base/security_headers.cpp` | **CERTIFIED** |
| **HSTS (Strict-Transport-Security)** | `net/http/transport_security_state.cc` | `third_party/chromium_net/base/security_headers.cpp` | **CERTIFIED** |
| **Navigation Scheme Whitelisting** | `content/browser/renderer_host/` | `kernel/apps/atrix/atrix_browser.c` | **CERTIFIED** |
| **Process Crash Containment** | `content/browser/child_process_launcher.cc` | `third_party/chromium_process/browser_process_host.cpp` | **CERTIFIED** |

---

## 2. Security Perimeter Architecture

```text
               INTERNET
                  │
                  ▼
          ┌───────────────┐
          │Network Process│  (PID N, No GUI / No Disk)
          └───────┬───────┘
                  │  Mojo IPC
                  ▼
          ┌───────────────┐
          │Browser Process│  (PID B, Windowing Authority)
          └───────┬───────┘
                  │  Mojo IPC
                  ▼
          ┌───────────────┐
          │Renderer (Tab) │  (PID R, UNTRUSTED)
          └───────┬───────┘
                  │
  ════════════════╪════════════════ (Kernel Boundary)
                  ▼
          ┌───────────────┐
          │ ATOMS Kernel  │  Syscall Filter: BLOCKED
          │    Sandbox    │  Capability: BOS_CAP_NONE
          └───────────────┘  Memory: W^X / NX Enforced
```
