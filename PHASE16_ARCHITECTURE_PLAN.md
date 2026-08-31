# PHASE 16 ARCHITECTURE PLAN: MEDIA / GPU / ADVANCED WEB APIS

**Document ID:** ATRIX-PHASE16-ARCH-001  
**Phase:** TASK 2 — ARCHITECTURE SPECIFICATION & INTEGRATION PLAN  
**Target Subsystem:** GPU Process, Chromium GPU Abstraction, WebGL (1.0), Canvas 2D / OffscreenCanvas, Media Pipeline, Kernel Audio HAL, Advanced Web APIs (Blob, File, Web Audio, MSE, WebCodecs)  
**Standard:** Rule 0 Phase Isolation Protocol (Investigate ➔ Plan ➔ Implement ➔ Build ➔ Runtime Verify ➔ Audit ➔ Certify)  
**Date:** 2026-08-26  
**Architect:** ATOMS OS Architecture & Graphics Committee  

---

## 1. System Architecture Overview

Phase 16 brings hardware/accelerated graphics, multimedia playback, and advanced HTML5 Web APIs to ATRIX Browser using the existing ATOMS OpenGL software rasterizer (`kernel/graphics/gl/`), BGL window surface manager (`kernel/graphics/bgl/`), and BOSPECTRA audio/video streaming engine (`kernel/media/bospectra/`).

```text
                                  ATRIX Browser
                                        │
                               ┌────────┴────────┐
                               ▼                 ▼
                        HTML / DOM / CSS   Canvas / WebGL / Media
                               │                 │
                               │ (Layout/Paint)  │
                               ▼                 ▼
                        Skia 2D Surface    Chromium GPU Abstraction
                               │                 │
                               │ (CPU Path)      │ Mojo IPC (Command Buffer)
                               │                 ▼
                               │           GPU Process (PID G)
                               │                 │
                               │                 ▼
                               │           ATOMS OpenGL (BGL)
                               │                 │
                               ▼                 ▼
                        BWE Window Surface / Present Queue
                               │
                               ▼
                        Desktop Display Framebuffer (1920x1080)
```

---

## 2. Component Architecture & Integration Specifications

### 2.1 GPU Process Architecture (`third_party/chromium_process/gpu_process_host.h`/`.cpp`)
- **Process Model:** Independent process with unique PID (`PID_GPU`) and hardware CR3 page table.
- **Sandboxing:** Bound to capability `BOS_CAP_GRAPHICS` (no raw disk write, no network access).
- **Communication:** Mojo message pipe connecting Browser Process Host and Renderer Process Hosts.
- **Crash Containment:** Monitored by `BrowserProcessHost`. If the GPU process crashes, the browser process logs the disconnect, isolates the failure, and falls back to CPU Skia rendering without crashing the browser UI.

### 2.2 Chromium GPU Abstraction & Command Buffer (`third_party/chromium_gpu/`)
- **`gpu::CommandBuffer`:** Circular ring buffer for serialized GPU commands.
- **`gpu::GpuCommandDecoder`:** Server-side decoder inside the GPU process that parses incoming commands, validates arguments (dimensions, offsets, lengths, buffer bounds), and dispatches calls to the ATOMS OpenGL engine.
- **Security Validation:** Rejects out-of-bounds vertex indices, negative viewport dimensions, and forged shared buffer handles.

### 2.3 WebGL 1.0 Integration (`third_party/blink/renderer/core/html/canvas/`)
- **`blink::WebGLRenderingContext`:** Full WebGL 1.0 JavaScript API implementation.
- **Core Operations:**
  - `clearColor(r, g, b, a)`, `clear(mask)`, `viewport(x, y, w, h)`.
  - `createShader(type)`, `compileShader(shader)`, `createProgram()`, `attachShader(prog, shader)`, `linkProgram(prog)`, `useProgram(prog)`.
  - `createBuffer()`, `bindBuffer(target, buf)`, `bufferData(target, data, usage)`.
  - `createTexture()`, `bindTexture(target, tex)`, `texImage2D(target, level, fmt, w, h, border, fmt, type, pixels)`.
  - `createFramebuffer()`, `bindFramebuffer(target, fbo)`, `framebufferTexture2D(target, attach, textarget, tex, level)`.
  - `drawArrays(mode, first, count)`, `drawElements(mode, count, type, offset)`.
  - `enable(cap)`, `disable(cap)`, `blendFunc(s, d)`, `depthFunc(func)`.
  - `isContextLost()`, `restoreContext()`.
- **WebGL 2.0 Status:** Formally documented as `UNSUPPORTED` (honest reporting per protocol).

### 2.4 Canvas 2D, OffscreenCanvas, ImageBitmap
- **`blink::CanvasRenderingContext2D`:**
  - Standard 2D drawing methods: `fillRect`, `strokeRect`, `clearRect`, `beginPath`, `moveTo`, `lineTo`, `stroke`, `fill`, `arc`, `drawImage`, `getImageData`, `putImageData`.
  - CPU Path: Skia 2D rendering into software bitmap surface.
  - GPU Path: OpenGL texture upload and composition.
- **`blink::OffscreenCanvas`:** Worker-thread accessible rasterization surface.
- **`blink::ImageBitmap`:** Zero-copy pixel buffer container.

### 2.5 Media Pipeline (`third_party/blink/renderer/core/html/media/`)
- **`blink::HTMLVideoElement` & `blink::HTMLAudioElement`:**
  - Playback State Machine: `HAVE_NOTHING` $\rightarrow$ `HAVE_METADATA` $\rightarrow$ `HAVE_ENOUGH_DATA`.
  - Controls: `play()`, `pause()`, `seek(time)`, `currentTime`, `duration`, `volume`, `muted`, `ended`.
  - Rendering: Video frames are decoded via BOSPECTRA pipeline and blitted to OpenGL texture or Skia surface.
  - Audio: PCM audio packets are routed to `kernel/audio/` (`audio_stream_write()`).

### 2.6 Advanced Web APIs
- **Blob & URL:** `blink::Blob`, `URL.createObjectURL(blob)`, `URL.revokeObjectURL(url)`.
- **File & FileReader:** `blink::File`, `blink::FileReader` (`readAsText`, `readAsDataURL`, `readAsArrayBuffer`).
- **Web Audio:** `blink::AudioContext`, `blink::GainNode`, `blink::AudioBufferSourceNode`.
- **MediaSource Extensions (MSE):** `blink::MediaSource`, `blink::SourceBuffer`.
- **WebCodecs Baseline:** `blink::VideoDecoder`, `blink::VideoFrame`, `blink::AudioDecoder`.

---

## 3. Comprehensive 44-Test Verification Plan (T01–T44)

| Test ID | Category | Target Subsystem / Vector | Expected Result |
|:---:|:---:|:---|:---|
| **T01** | GPU | OpenGL Context Creation (`wglCreateContext` / `bglCreateContext`) | Context handle $> 0$ |
| **T02** | GPU | OpenGL Capability & String Detection | Returns vendor & OpenGL 2.0 version |
| **T03** | GPU | Shader Compilation / State Setup | Valid shader ID created |
| **T04** | GPU | Vertex Buffer Allocation & Data Upload | Buffer ID generated, data stored |
| **T05** | GPU | Texture Allocation & Image Upload (`glTexImage2D`) | Texture generated and bound |
| **T06** | GPU | Framebuffer Object Creation (`glGenFramebuffers`) | FBO generated and complete |
| **T07** | GPU | GPU Buffer Clear (`glClearColor` / `glClear`) | Framebuffer color set |
| **T08** | GPU | GPU Draw Call (`glDrawArrays` / `glDrawElements`) | Primitives rasterized |
| **T09** | GPU | GPU Frame Presentation to Display (`bglSwapBuffers`) | Backbuffer swapped to BWE surface |
| **T10** | GPU | GPU Process Isolation | Separate PID & CR3 from Browser/Renderer |
| **T11** | WebGL | WebGL 1.0 Context Creation (`getContext('webgl')`) | WebGL context object returned |
| **T12** | WebGL | WebGL Shader Execution & Program Link | Program links successfully |
| **T13** | WebGL | WebGL Vertex Buffer Rendering | Geometry drawn to WebGL buffer |
| **T14** | WebGL | WebGL Texture Rendering | Texture mapped to quad |
| **T15** | WebGL | WebGL Framebuffer Rendering | Offscreen FBO render target valid |
| **T16** | WebGL | WebGL Context Loss Simulation | `isContextLost()` returns true |
| **T17** | WebGL | WebGL Context Restoration | Context successfully recreated |
| **T18** | Canvas | Canvas 2D Context Creation (`getContext('2d')`) | `CanvasRenderingContext2D` valid |
| **T19** | Canvas | Canvas 2D Path, Fill & Image Output | Pixel buffer rendered cleanly |
| **T20** | Canvas | OffscreenCanvas Rendering | Background thread surface rendered |
| **T21** | Canvas | ImageBitmap Extraction & Transfer | Zero-copy bitmap container valid |
| **T22** | Media | `HTMLVideoElement` Lifecycle & Properties | `video.videoWidth` & `video.duration` valid |
| **T23** | Media | `HTMLAudioElement` Lifecycle & Properties | Audio element initialized |
| **T24** | Media | Media Play / Pause State Machine | Play state toggles cleanly |
| **T25** | Media | Media Seeking (`currentTime = 5.0`) | Timestamp jumps cleanly |
| **T26** | Media | Media Buffering Progress | `buffered` range advances |
| **T27** | Media | Video Frame Rendering to Surface | Video frame painted to screen |
| **T28** | Media | Audio PCM Stream Output | PCM samples sent to audio HAL |
| **T29** | Media | Media Process Isolation | Media tasks isolated from renderer |
| **T30** | Media | Media Crash Containment | Media error handled safely |
| **T31** | Advanced | Blob Creation & `URL.createObjectURL()` | Valid `blob:https://...` generated |
| **T32** | Advanced | `File` & `FileReader` Text / Data Extraction | File content read accurately |
| **T33** | Advanced | Web Audio `AudioContext` & Graph Routing | Audio graph computes gain correctly |
| **T34** | Advanced | `MediaSource` & `SourceBuffer` Appending | Media segment appended to buffer |
| **T35** | Advanced | `WebCodecs` VideoDecoder Chunk Decoding | Raw frame extracted |
| **T36** | Security | Invalid GPU Handle Rejection | Forged context/texture handle rejected |
| **T37** | Security | Invalid Command Buffer Stream Rejection | Truncated/corrupt commands rejected |
| **T38** | Security | Shared Memory Bounds Validation | Out-of-bounds framebuffer write blocked |
| **T39** | Security | Unauthorized Direct GPU Syscall Rejection | Sandboxed renderer direct access denied |
| **T40** | Security | Renderer $\rightarrow$ GPU Process Memory Isolation | Distinct CR3 address spaces |
| **T41** | Regression| Phase 15 Security Sandbox Validation | All Phase 15 guarantees 100% active |
| **T42** | Regression| Phase 14 Mojo IPC Transport Validation | All 28 Mojo tests 100% passing |
| **T43** | Regression| Phase 13 Multi-Process Browser Isolation | Browser, Renderer, Net processes intact |
| **T44** | Regression| Phase 1–12 Browser Engine Regression | DOM, CSS, Skia CPU, Network functional |

---

## 4. Rollback & Fail-Safe Plan

1. If GPU initialization fails on a specific hardware platform or virtual machine, ATRIX Browser automatically falls back to certified Skia CPU rendering.
2. If audio hardware is absent (e.g. QEMU without `-soundhw`), `kernel/audio/` falls back to virtual / null sink without blocking the browser engine.
3. No breaking changes to existing `bgl.c`, `gl.c`, or `vmm.c`.
