# PHASE 12 ARCHITECTURE PLAN: CHROMIUM NETWORKING + BROWSER STORAGE

**Document ID:** ATRIX-PHASE12-PLAN-001  
**Phase:** STEP 2 — ARCHITECTURAL PLAN & PATCH SPECIFICATION  
**Target Subsystem:** Chromium Networking (`net/`), URL Loading, Cookies, HTTP Cache, Browser Storage, Origin Security & ATOMS Platform Adapters  
**Standard:** Rule 0 Phase Isolation (Investigate ➔ Plan ➔ Implement ➔ Build ➔ Runtime Verify ➔ Forensic Audit ➔ Certify)  
**Date:** 2026-08-26  

---

## 1. Architectural Blueprint & Subsystem Layers

```text
┌────────────────────────────────────────────────────────────────────────┐
│                        ATRIX BROWSER / BLINK DOM                       │
│           (Document, Window, XMLHttpRequest, fetch, localStorage)       │
├────────────────────────────────────────────────────────────────────────┤
│                     CHROMIUM NETWORKING & STORAGE CORE                 │
│  ┌──────────────────────────────────────────────────────────────────┐  │
│  │ GURL / KURL (Canonicalization, Scheme, Host, Port Parsing)       │  │
│  │                                                                  │  │
│  │ SecurityOrigin (Origin Isolation: scheme://host:port)            │  │
│  │                                                                  │  │
│  │ CookieStore / CookieMonster (RFC 6265 Cookie Matching & Storage) │  │
│  │                                                                  │  │
│  │ HttpCache (Memory/Disk Caching, ETag / Revalidation)            │  │
│  │                                                                  │  │
│  │ URLLoader / URLRequest (HTTP 1.1 / TLS Request Engine)           │  │
│  │                                                                  │  │
│  │ DOMStorage (LocalStorage, SessionStorage, VFS Persistence)       │  │
│  └───────────────────────────────────┬──────────────────────────────┘  │
├──────────────────────────────────────┼─────────────────────────────────┤
│                                      ▼                                 │
│                     ATOMS PLATFORM ADAPTER LAYER                       │
│  ┌──────────────────────────────────────────────────────────────────┐  │
│  │ AtomsNetworkAdapter (Bridges URLLoader to Socket/TLS ABI)        │  │
│  │ AtomsStorageAdapter (Bridges DOMStorage to ATOMS VFS / Disk)     │  │
│  └───────────────────────────────────┬──────────────────────────────┘  │
├──────────────────────────────────────┼─────────────────────────────────┤
│                                      ▼                                 │
│                  ATOMS OS KERNEL NETWORKING & VFS                      │
│            (DNS Resolver, TCP/IP Stack, TLS 1.2 ECDHE, VFS)            │
└────────────────────────────────────────────────────────────────────────┘
```

---

## 2. File Implementation & Creation Directory Plan

### 2.1 Chromium Net Subsystem (`third_party/chromium_net/`)
- `base/gurl.h` & `base/gurl.cpp`: Full URL parser, canonicalization, port normalization, origin derivation.
- `base/security_origin.h` & `base/security_origin.cpp`: `(scheme, host, port)` tuple comparison, same-origin enforcement, serialization.
- `http/http_request_headers.h` & `http/http_request_headers.cpp`: Header map management, serialization to wire format (`Header: Value\r\n`).
- `http/http_response_headers.h` & `http/http_response_headers.cpp`: Status code parsing (200, 301, 302, 404, 500), header extraction, content length, redirect detection.
- `cookies/canonical_cookie.h` & `cookies/canonical_cookie.cpp`: Cookie representation, domain matching, path matching, `Secure`, `HttpOnly`, expiration.
- `cookies/cookie_store.h` & `cookies/cookie_store.cpp`: Origin cookie store, `SetCookie`, `GetCookieHeaderForURL`, persistence.
- `http/http_cache.h` & `http/http_cache.cpp`: URL-keyed response cache, expiration calculation, revalidation headers (`If-None-Match`, `If-Modified-Since`).
- `url_request/url_loader.h` & `url_request/url_loader.cpp`: High-level network request dispatcher, redirect follower, cookie updater, cache reader/writer.
- `adapter/atoms_network_adapter.h` & `adapter/atoms_network_adapter.cpp`: Bridges `URLLoader` directly to ATOMS kernel networking (`abe_net_manager`, `abe_net_conn`, `abe_net_tls`, `abe_net_dns`).

### 2.2 Chromium Browser Storage Subsystem (`third_party/chromium_storage/`)
- `dom_storage/storage_area.h` & `dom_storage/storage_area.cpp`: Key-value storage dictionary, quota tracking (5MB per origin), modification listeners.
- `dom_storage/storage_namespace.h` & `dom_storage/storage_namespace.cpp`: Origin-partitioned namespace registry ensuring strict separation.
- `dom_storage/local_storage_manager.h` & `dom_storage/local_storage_manager.cpp`: Persistent storage manager writing records to ATOMS VFS.
- `dom_storage/session_storage_manager.h` & `dom_storage/session_storage_manager.cpp`: In-memory session storage manager.
- `adapter/atoms_storage_vfs_adapter.h` & `adapter/atoms_storage_vfs_adapter.cpp`: Atomic file serialization and deserialization over ATOMS VFS (`/var/storage/`).

### 2.3 Blink & V8 Bindings (`third_party/blink/renderer/core/bindings/core/v8/`)
- Extend `script_controller.cpp` to expose:
  - `window.localStorage` (`setItem`, `getItem`, `removeItem`, `clear`, `length`, `key`).
  - `window.sessionStorage` (`setItem`, `getItem`, `removeItem`, `clear`).
  - `window.fetch(url)`: Asynchronously dispatches `URLLoader` and returns response.

### 2.4 Test Suite & Standalone Runner (`third_party/chromium_net/tests/`)
- `net_storage_test_suite.h` & `net_storage_test_suite.cpp`: 34 comprehensive deterministic verification tests covering Network, Cookies, Cache, Storage, Origin Isolation, and Blink/V8 Integration.
- `net_storage_test_main.cpp`: Standalone ELF executable runner.
