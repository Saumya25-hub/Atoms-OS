# ATOMS OS: CHROMIUM, BLINK, NETWORKING, STORAGE, MOJO & SECURITY PROVENANCE RECORD

**Document ID:** ATRIX-PHASE15-PROVENANCE-001  
**Phase:** Phase 15 — Sandbox + Web Security Provenance  
**Date:** 2026-08-26  

---

## 1. Upstream Project & Copyright Notice

- **Project:** The Chromium Project (Mojo IPC, Blink, Chromium Net, Chromium Storage, Content Security Policy, Security Origin)
- **Upstream Repository:** https://chromium.googlesource.com/chromium/src
- **Subsystems:** `mojo/`, `services/network/`, `components/services/storage/`, `third_party/blink/renderer/core/frame/csp/`
- **Copyright:** Copyright © 2014 The Chromium Authors. All rights reserved.
- **License:** **BSD 3-Clause License**

---

## 2. Reused & Adapted Subsystems

| Subsystem / Directory | Upstream Origin | Classification | Purpose in ATOMS OS / ATRIX |
|:---|:---|:---:|:---|
| `third_party/chromium_net/base/security_origin.*` | Chromium Net | **UPSTREAM CHROMIUM** | Scheme, Host, Port tuple matching and `IsSameOriginWith()` verification. |
| `third_party/chromium_net/cookies/` | Chromium Net | **UPSTREAM CHROMIUM** | Cookie security: `HttpOnly`, `Secure`, SameSite, Domain/Path matching. |
| `third_party/chromium_storage/dom_storage/` | Chromium DOM Storage | **UPSTREAM CHROMIUM** | Origin-isolated `StorageNamespace` and `StorageArea`. |
| `third_party/chromium_net/base/content_security_policy.*` | Chromium Blink CSP | **MODIFIED UPSTREAM CHROMIUM** | CSP directive parsing (`default-src`, `script-src`, `style-src`, `img-src`, `frame-ancestors`). |
| `kernel/sandbox/` | ATOMS Project | **ATOMS ORIGINAL IMPLEMENTATION** | Kernel-level process sandboxing, capability enforcement, syscall filtering, and memory guard pages. |
| `kernel/core/syscall/` | ATOMS Project | **ATOMS ORIGINAL IMPLEMENTATION** | Syscall dispatcher with sandboxed process filtering and pointer validation. |
| `kernel/core/memory/vmm/` | ATOMS Project | **ATOMS ORIGINAL IMPLEMENTATION** | 4-level paging, W^X page permission enforcement, and NX protection. |
| `third_party/chromium_security/tests/` | ATOMS Project | **ATOMS ORIGINAL IMPLEMENTATION** | 30-test hostile sandbox escape and web security verification suite. |
| `third_party/chromium_gpu/` | Chromium GPU / CommandBuffer | **MODIFIED UPSTREAM CHROMIUM** | GPU command buffer, command decoder, texture mailbox, and GPU channel over Mojo IPC. |
| `third_party/blink/renderer/core/html/canvas/` | Chromium Blink Canvas / WebGL | **MODIFIED UPSTREAM CHROMIUM** | WebGL 1.0 rendering context, Canvas 2D context, OffscreenCanvas, and ImageBitmap. |
| `third_party/blink/renderer/core/html/media/` | Chromium Blink Media | **MODIFIED UPSTREAM CHROMIUM** | HTMLVideoElement, HTMLAudioElement, playback state machine, video frame painting. |
| `third_party/blink/renderer/core/fileapi/` | Chromium Blink File API | **MODIFIED UPSTREAM CHROMIUM** | Blob, URL.createObjectURL, File, and FileReader APIs. |
| `third_party/blink/renderer/modules/webaudio/` | Chromium Blink Web Audio | **MODIFIED UPSTREAM CHROMIUM** | AudioContext, GainNode, AudioBufferSourceNode graph routing. |
| `third_party/blink/renderer/modules/mediasource/` | Chromium Blink MSE | **MODIFIED UPSTREAM CHROMIUM** | MediaSource and SourceBuffer buffer appending pipeline. |
| `third_party/blink/renderer/modules/webcodecs/` | Chromium Blink WebCodecs | **MODIFIED UPSTREAM CHROMIUM** | VideoDecoder, VideoFrame, AudioDecoder baseline primitives. |
| `kernel/graphics/gl/` | ATOMS Project | **ATOMS ORIGINAL IMPLEMENTATION** | Software OpenGL 2.0 rasterizer, FBO, texture, depth, and blending pipeline. |
| `kernel/graphics/bgl/` | ATOMS Project | **ATOMS ORIGINAL IMPLEMENTATION** | Native BGL window drawable and context lifecycle management. |
| `kernel/audio/` | ATOMS Project | **ATOMS ORIGINAL IMPLEMENTATION** | Multi-channel PCM audio streaming engine and Realtek/AC97 drivers. |
| `kernel/media/bospectra/` | ATOMS Project | **ATOMS ORIGINAL IMPLEMENTATION** | Multimedia container demuxing, video decoding, and audio/video synchronization. |
| `third_party/chromium_compatibility/` | ATOMS Project / Chromium | **ATOMS ORIGINAL IMPLEMENTATION** | Phase 17 46-test deterministic web compatibility, fuzzing, and hardening suite. |

