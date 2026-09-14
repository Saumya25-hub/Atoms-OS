# PHASE 17 ARCHITECTURE & HARDENING PLAN

**Document ID:** ATRIX-PHASE17-PLAN-001  
**Phase:** TASK 2 — ARCHITECTURE PLAN & HARDENING SPECIFICATION  
**Target Subsystems:** HTML/CSS/DOM Engine, V8 Bindings, Mojo IPC, Storage, GPU/Media, Network Process, Fuzzing & Malformed Input Engine, Crash Containment & Resource Limits  
**Standard:** Rule 0 Phase Isolation Protocol (Investigate ➔ Plan ➔ Implement ➔ Build ➔ Runtime Verify ➔ Audit ➔ Certify)  
**Date:** 2026-08-26  
**Auditor:** ATOMS OS Web Platform & Browser Security Architecture Committee  

---

## 1. Objectives & Scope

Phase 17 performs end-to-end hardening, standards compliance verification, hostile fuzz testing, resource exhaustion protection, and memory leak stress testing across the entire browser stack.

```text
               Untrusted Web Content / Fuzz Inputs
                               │
            ┌──────────────────┼──────────────────┐
            ▼                  ▼                  ▼
     Malformed HTML       Hostile CSS       Corrupt JavaScript
            │                  │                  │
            ▼                  ▼                  ▼
   HTML Parser Guard   CSS Parser Guard    V8 Sandbox Guard
   (Max Depth: 128)    (Safe Recovery)    (Bounds Checked)
            │                  │                  │
            └──────────────────┼──────────────────┘
                               │
                               ▼
                   Mojo IPC Message Validator
                   (Max 1MB, Handles Verified)
                               │
            ┌──────────────────┼──────────────────┐
            ▼                  ▼                  ▼
     GPU Command Guard   Network Validator   Storage Guard
     (Handles/Bounds)    (CORS/Origin/TLS)   (10MB Quota/VFS)
                               │
                               ▼
                   Process Crash Containment
               (Browser Process Survives Always)
```

---

## 2. Hardening Specifications

### 2.1 Parser Fuzzing & Safe Error Recovery
- **HTML Parser:** Enforce recursion depth limit of 128 nested elements to prevent stack exhaustion. Handle unterminated strings, orphan closing tags, missing quotes, and mixed case tags.
- **CSS Parser:** Ignore invalid property names or malformed values without failing the entire stylesheet. Recover cleanly on missing semicolons and unclosed braces.
- **URL Parser:** Reject malformed URLs, control characters in hostnames, and invalid port numbers ($>65535$).

### 2.2 IPC & Shared Memory Bounds Protection
- **Mojo Serialization:** Strictly enforce 1MB maximum message payload size. Reject serialized messages with payload length mismatch or invalid handle indices.
- **Shared Memory:** Validate offset $+$ length $\le$ buffer capacity on all mapping requests; return `MOJO_RESULT_OUT_OF_RANGE` on violation.

### 2.3 Resource Exhaustion Guards
- **DOM Size Limits:** Max 10,000 DOM nodes per document.
- **Canvas / WebGL Limits:** Max canvas dimensions $8192 \times 8192$; max texture dimensions $4096 \times 4096$.
- **Storage Limits:** Max 10MB per origin for localStorage; max 5MB for sessionStorage.

### 2.4 Multi-Process Crash Containment
- When a child process (Renderer, GPU, Network, Utility) crashes:
  1. The Browser Process receives process exit notification.
  2. Handle table entries associated with the dead process are reclaimed.
  3. Shared memory buffers are unmapped.
  4. The Browser Process remains fully operational and responsive.

---

## 3. Compatibility & Hardening Verification Test Plan (T01 – T46)

| Test ID | Category | Target Subsystem | Expected Verdict |
|:---:|:---|:---|:---:|
| **T01** | HTML | Basic HTML document parsing & DOM tree generation | **PASS** |
| **T02** | HTML | Malformed HTML error recovery (omitted/unclosed tags) | **PASS** |
| **T03** | DOM | DOM mutation (`createElement`, `appendChild`, `removeChild`, `insertBefore`) | **PASS** |
| **T04** | CSS | CSS selector matching (`#id`, `.class`, `tag`) | **PASS** |
| **T05** | CSS | CSS cascade & specificity calculation $(a, b, c)$ | **PASS** |
| **T06** | CSS | CSS property inheritance (`color`, `font-size`) | **PASS** |
| **T07** | CSS | Box model geometry (content, padding, border, margin) | **PASS** |
| **T08** | CSS | Responsive media query evaluation | **PASS** |
| **T09** | JS | JavaScript execution via Google V8 engine | **PASS** |
| **T10** | JS | DOM + JavaScript integration (`innerHTML`, `textContent`, `title`) | **PASS** |
| **T11** | Events | Event handling (`addEventListener`, `click`, bubbling) | **PASS** |
| **T12** | Forms | Form input serialization and submission | **PASS** |
| **T13** | Network | URL parsing and validation (`GURL`) | **PASS** |
| **T14** | Network | HTTPS / TLS secure connection negotiation | **PASS** |
| **T15** | Network | HTTP redirect handling (301 / 302 / 307 / 308) | **PASS** |
| **T16** | Network | Cookie management (`CanonicalCookie`, `HttpOnly`, `Secure`) | **PASS** |
| **T17** | Network | HTTP caching (`ETag`, `Last-Modified`, `304 Not Modified`) | **PASS** |
| **T18** | Storage | `localStorage` persistent storage and origin isolation | **PASS** |
| **T19** | Storage | `sessionStorage` tab-scoped in-memory storage | **PASS** |
| **T20** | Security | Same-Origin Policy (SOP) cross-origin access prevention | **PASS** |
| **T21** | Canvas | Canvas 2D rasterization via Skia CPU path | **PASS** |
| **T22** | WebGL | WebGL 1.0 hardware rendering pipeline | **PASS** |
| **T23** | WebGL | WebGL context loss and restoration event cycle | **PASS** |
| **T24** | Media | HTML5 `<video>` playback state machine and frame render | **PASS** |
| **T25** | Media | HTML5 `<audio>` playback routed to ATOMS audio HAL | **PASS** |
| **T26** | Media | Web Audio API node graph construction and sample stream | **PASS** |
| **T27** | Media | MediaSource Extensions (MSE) buffer append and stream end | **PASS** |
| **T28** | Media | WebCodecs VideoDecoder configuration and frame output | **PASS** |
| **T29** | APIs | Blob creation, `URL.createObjectURL`, and FileReader | **PASS** |
| **T30** | Mojo | Mojo message pipe and shared buffer IPC communication | **PASS** |
| **T31** | Crash | Renderer process crash containment | **PASS** |
| **T32** | Crash | GPU process crash containment | **PASS** |
| **T33** | Crash | Network process crash containment | **PASS** |
| **T34** | Crash | Utility process crash containment | **PASS** |
| **T35** | Fuzz | HTML parser malformed/fuzz input safety | **PASS** |
| **T36** | Fuzz | CSS parser corrupt/fuzz input safety | **PASS** |
| **T37** | Fuzz | URL parser malformed scheme/host/port fuzz safety | **PASS** |
| **T38** | Fuzz | HTTP header corrupt/oversized fuzz safety | **PASS** |
| **T39** | Fuzz | Mojo IPC malformed serialization fuzz safety | **PASS** |
| **T40** | Fuzz | Corrupt storage record recovery | **PASS** |
| **T41** | Fuzz | GPU command buffer fuzz & out-of-bounds safety | **PASS** |
| **T42** | Resource | Resource exhaustion protection (DOM depth, canvas size) | **PASS** |
| **T43** | Lifecycle | Repeated page creation, navigation, and teardown cycle | **PASS** |
| **T44** | Stress | Multi-tab concurrent workload and memory stability | **PASS** |
| **T45** | Security | Phase 15 sandbox security regression validation | **PASS** |
| **T46** | Regression | Phases 1–14 core browser foundations regression | **PASS** |

---

## 4. Implementation Files

- Implementation of test suite:
  - [`third_party/chromium_compatibility/tests/compatibility_test_suite.h`](file:///D:/Signatures_OS/third_party/chromium_compatibility/tests/compatibility_test_suite.h)
  - [`third_party/chromium_compatibility/tests/compatibility_test_suite.cpp`](file:///D:/Signatures_OS/third_party/chromium_compatibility/tests/compatibility_test_suite.cpp)
  - [`third_party/chromium_compatibility/tests/compatibility_test_main.cpp`](file:///D:/Signatures_OS/third_party/chromium_compatibility/tests/compatibility_test_main.cpp)
- Build System targets:
  - [`BUILD.gn`](file:///D:/Signatures_OS/BUILD.gn) (`compatibility_test_runner` executable target)
  - [`build.ps1`](file:///D:/Signatures_OS/build.ps1) (Master OS build integration)
- Browser Diagnostics:
  - [`kernel/apps/atrix/atrix_browser.c`](file:///D:/Signatures_OS/kernel/apps/atrix/atrix_browser.c) (routes `about:compat`, `about:fuzz`, `about:stress`)

---

## 5. Rollback Plan

If any test fails during execution, changes are isolated strictly to the new compatibility test suite and hardening wrapper functions. Core kernel functions remain untouched.
