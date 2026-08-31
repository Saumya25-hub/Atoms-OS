# PHASE 15 ARCHITECTURE PLAN: SANDBOX + WEB SECURITY

**Document ID:** ATRIX-PHASE15-ARCH-001  
**Phase:** TASK 3 — ARCHITECTURAL SPECIFICATION & DESIGN PLAN  
**Target Subsystem:** Kernel Sandbox Engine, Process Capabilities, Syscall Filter, Memory Protection (W^X / NX / Guard Pages), IPC/Mojo Security, Web Origin Isolation, Cookie Security, Storage Security, Navigation Security, CSP & Threat Mitigation  
**Standard:** Rule 0 Phase Isolation Protocol (Investigate ➔ Plan ➔ Implement ➔ Build ➔ Runtime Verify ➔ Audit ➔ Certify)  
**Date:** 2026-08-26  
**Architect:** ATOMS OS Security & Architecture Committee  

---

## 1. Executive Architectural Overview

Phase 15 establishes a **kernel-enforced, multi-tier security perimeter** around untrusted web content in ATRIX on ATOMS OS:

```text
                                  INTERNET
                                     │
                                     ▼
                            ┌─────────────────┐
                            │ Network Process │  (PID N, Sandboxed: No GUI, No Disk Storage)
                            └────────┬────────┘
                                     │
                                 Mojo IPC
                                     │
                                     ▼
                            ┌─────────────────┐
                            │ Browser Process │  (PID B, Unsandboxed: Windowing & Shell)
                            └────────┬────────┘
                                     │
                                 Mojo IPC
                                     │
                                     ▼
                            ┌─────────────────┐
                            │Renderer Process │  (PID R, STRICT KERNEL SANDBOX)
                            │   (UNTRUSTED)   │  (W^X, NX, No Raw VFS, No Raw Sockets)
                            └────────┬────────┘
                                     │
                       ══════════════╪══════════════ (Kernel Boundary)
                                     ▼
                     ┌────────────────────────────────┐
                     │     ATOMS Kernel Sandbox       │
                     │  - Syscall Policy Filter       │
                     │  - Capability Token Engine     │
                     │  - VMM W^X / NX Enforcement    │
                     │  - Memory Bounds Validation    │
                     └────────────────────────────────┘
```

---

## 2. Kernel-Enforced Capability Model

1. **Capability Token Definition:**
   - Every process is assigned an unforgeable `bos_token_t` with a 64-bit capability bitmask.
   - For `ABE_PROC_ROLE_RENDERER`, capabilities are initialized to `BOS_CAP_NONE`.
2. **Capability Taxonomy:**
   - `BOS_CAP_READ_FILES` (0x1) — Disallowed for Renderer.
   - `BOS_CAP_WRITE_FILES` (0x2) — Disallowed for Renderer.
   - `BOS_CAP_DELETE_FILES` (0x4) — Disallowed for Renderer.
   - `BOS_CAP_NETWORK` (0x8) — Disallowed for Renderer (routed via Network Process).
   - `BOS_CAP_TEMP_STORAGE` (0x800) — Allowed for internal scratch allocation only.
   - `BOS_CAP_USB` / `BOS_CAP_SYS_SETTINGS` — Strictly prohibited for Renderer.

---

## 3. Syscall Sandbox Policy for Untrusted Renderers

| Syscall Number | Syscall Name | Policy for Sandboxed Renderer | Kernel Validation Rule |
|:---:|:---|:---:|:---|
| **0** | `SYS_WRITE` | **CONTROLLED** | Stdout debug only; length capped to 1024 bytes. |
| **1** | `SYS_EXIT` | **ALLOWED** | Process voluntary exit; triggers cleanup of handles & memory. |
| **2** | `SYS_GETPID` | **ALLOWED** | Read-only identity check. |
| **3** | `SYS_YIELD` | **ALLOWED** | CPU scheduling yield. |
| **4** | `SYS_UPTIME` | **ALLOWED** | Monotonic clock read. |
| **5** | `SYS_ALLOC` | **CONTROLLED** | Heap allocation up to process quota (128 MB max). |
| **6** | `SYS_FREE` | **ALLOWED** | Free allocated memory. |
| **8** | `SYS_MMAP` | **CONTROLLED** | Must enforce W^X (no `PROT_WRITE \| PROT_EXEC`). |
| **9** | `SYS_MUNMAP` | **ALLOWED** | Memory unmap within user virtual bounds. |
| **10** | `SYS_MPROTECT` | **CONTROLLED** | Rejects transitions to simultaneously writable & executable (`RWX`). |
| **11** | `SYS_FUTEX` | **ALLOWED** | Thread synchronization. |
| **12** | `SYS_CLOCK_GETTIME` | **ALLOWED** | Timing primitives. |
| **13** | `SYS_NANOSLEEP` | **ALLOWED** | Sleep primitives. |
| **14** | `SYS_OPEN` | **DENIED** | Returns `SYSCALL_FAIL` / `BOS_SANDBOX_ERR_PATH_DENIED`. |
| **15** | `SYS_READ` | **DENIED** | Returns `SYSCALL_FAIL` / `BOS_SANDBOX_ERR_SYSCALL_BLOCKED`. |
| **16–24** | `SYS_GUI_*` | **DENIED** | Direct window creation/mutation blocked for Renderer. |
| **25** | `SYS_CLOSE` | **DENIED** | Direct VFS file descriptor close blocked. |
| **29** | `SYS_WRITE_FILE` | **DENIED** | Direct file write blocked. |
| **$\ge 100$** | Privileged Syscalls | **DENIED** | Immediately blocked; triggers security audit log. |

---

## 4. Virtual Memory Security (W^X & Address Space Bounds)

1. **W^X (Write XOR Execute) Rule:**
   - Any page table entry where `VMM_FLAG_WRITABLE` is set MUST have `VMM_FLAG_NX` (Bit 63) set.
   - Memory pages mapped with execution rights (`PROT_EXEC`) MUST NOT have `VMM_FLAG_WRITABLE` set.
   - Any attempt by JIT or malicious script to call `mprotect()` with `PROT_WRITE | PROT_EXEC` is rejected with `BOS_SANDBOX_ERR_MEMORY_VIOLATION`.
2. **User vs Kernel Address Space Isolation:**
   - `VMM_USER_MIN_ADDRESS = 0x0000000001000000ULL` (Trap NULL-pointer dereferences in the lower 16MB).
   - `VMM_USER_MAX_ADDRESS = 0x00007FFFFFFFFFFFULL` (Standard canonical user ceiling).
   - Kernel memory base: `0xFFFF800000000000ULL` (Protected by hardware supervisor bit `U/S = 0`).

---

## 5. Web Origin & Same-Origin Policy (SOP)

1. **Origin Tuple:** `(Scheme, Host, Port)`.
   - `https://example.com:443` $\neq$ `http://example.com:80` (Different scheme).
   - `https://api.example.com:443` $\neq$ `https://example.com:443` (Different host).
   - `https://example.com:8443` $\neq$ `https://example.com:443` (Different port).
2. **DOM Isolation:** Cross-origin script execution cannot access or mutate DOM nodes across frames.
3. **Storage Isolation:** `LocalStorage` and `SessionStorage` namespaces are partitioned strictly by `SecurityOrigin::ToString()`.

---

## 6. Cookie Security Engine

1. **`HttpOnly` Flag:**
   - Cookies marked `HttpOnly` are stored in the Network/Storage process.
   - `document.cookie` in Blink / JS cannot read `HttpOnly` cookies.
2. **`Secure` Flag:**
   - Cookies marked `Secure` are only transmitted over `https://` URLs.
   - Attempts to send `Secure` cookies over plaintext `http://` are rejected.

---

## 7. Content Security Policy (CSP) & Web Headers

1. **Directives Supported:**
   - `default-src <sources>`: Fallback for all resource types.
   - `script-src <sources>`: Controls allowed script execution domains (`'self'`, `'unsafe-inline'`, `https://...`).
   - `style-src <sources>`: Controls allowed stylesheet sources.
   - `img-src <sources>`: Controls allowed image sources.
   - `frame-ancestors <sources>`: Restricts framing (clickjacking prevention).
2. **Additional Headers:**
   - `X-Frame-Options: DENY | SAMEORIGIN`.
   - `Strict-Transport-Security: max-age=<seconds>`.
   - `Referrer-Policy: no-referrer | strict-origin-when-cross-origin`.

---

## 8. Navigation & Scheme Whitelisting

- **Allowed Schemes:** `http:`, `https:`, `about:`, `data:`.
- **Prohibited / Blocked Schemes:** `javascript:`, `file:`, `vbscript:`, `shell:`, unknown custom schemes.
- Attempts to navigate to prohibited schemes are intercepted and rejected with `ERR_DISALLOWED_URL_SCHEME`.

---

## 9. Implementation Plan & File Additions

| Component | File Path | Purpose |
|:---|:---|:---|
| **CSP Engine** | `third_party/chromium_net/base/content_security_policy.h`<br>`content_security_policy.cpp` | CSP header parser and directive evaluation engine. |
| **Security Origin Enhancements** | `third_party/chromium_net/base/security_origin.h`<br>`security_origin.cpp` | Enhanced origin isolation and cross-origin checks. |
| **Renderer Sandbox Integration** | `third_party/chromium_process/renderer_process_host.cpp`<br>`third_party/chromium_process/browser_process_host.cpp` | Attaching sandbox context, verifying capability masks, and enforcing Mojo message validation. |
| **Kernel Sandbox Hook** | `kernel/browser_engine/process/abe_process.c`<br>`kernel/sandbox/syscall/sandbox_syscall.c` | Automatic sandbox context binding for renderer processes. |
| **Hostile Security Test Suite** | `third_party/chromium_security/tests/security_test_suite.h`<br>`security_test_suite.cpp`<br>`security_test_main.cpp` | 30 deterministic hostile security tests (T01–T30). |
| **Build Integration** | `BUILD.gn`<br>`build.ps1` | Build targets and linker integration. |

---

## 10. Rollback Plan

The Phase 14 Mojo and Phase 13 multi-process foundations remain fully intact. In the event of a security regression, the sandbox hooks can be toggled without disturbing kernel core initialization.
