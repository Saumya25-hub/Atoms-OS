# BOS OS — Phase 3: Production TLS & Security Engine

## Overview
The BOS OS Security Engine is a production-grade cryptographic and secure networking subsystem written in freestanding C for x86_64 architecture. It provides high-assurance cryptographic primitives, certificate validation, trust store anchor management, and full TLS 1.2/1.3 protocol capabilities for the kernel, ATRIX Browser, package manager, and secure software update infrastructure.

## Directory Layout
- `include/`: Public and internal C headers for all security subsystems.
- `core/`: Lifecycle orchestrator and initialization state (`bos_security_init()`).
- `crypto/`: HMAC-SHA256/384/512 and HKDF-Extract/HKDF-Expand engines.
- `random/`: Hardware TSC entropy mixer and CSPRNG.
- `hash/`: FIPS 180-4 standard SHA-256, SHA-384, and SHA-512 engines.
- `aes/`: FIPS 197 standard AES-128 and AES-256 (ECB, CBC, CTR, GCM modes).
- `rsa/`: RSA-2048/4096 PKCS#1 v1.5 signing, verification, encryption, and decryption.
- `ecc/`: NIST P-256 (secp256r1) ECDSA signature and ECDHE key exchange engine.
- `x509/`: ASN.1 DER parser for X.509 certificates and SAN extensions.
- `certificates/`: Certificate chain and wildcard domain validator.
- `trust_store/`: Root CA and intermediate certificate trust registry.
- `session/`: TLS session ticket and session ID cache with TTL eviction.
- `tls/`: TLS record layer, handshake state machine, and cipher suite negotiator.
- `diagnostics/`: Cryptographic operation metrics and performance counter engine.
- `debug/`: Isolated trace logger with configurable bitmasks.
- `tests/`: 30-item automated certification test suite.

## Verification & Status
All 30 automated certification tests pass inside live QEMU boot execution.
