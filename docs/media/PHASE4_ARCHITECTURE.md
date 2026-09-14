# ATOMS OS — Phase 4 Architecture Specification
**Subsystem:** Native Media Engine ➔ Time-Aware SIMD & Low-Copy Video Pipeline  
**Milestone:** Phase 4B (Architectural Blueprint)  
**Date:** September 12, 2026  
**Status:** **APPROVED FOR IMPLEMENTATION PLANNING (RULE 0 ENFORCED — NO CODE MODIFIED)**  

---

## 1. Architectural Mission & Principles

The Phase 4 architecture transforms the ATOMS OS Media Subsystem into a **time-aware, data-aware, bounded, SIMD-accelerated real-time computation pipeline**.

```
                           +-------------------------------------+
                           |         BOS Media Clock             |
                           |  (Monotonic System Uptime Microsec) |
                           +------------------+------------------+
                                              |
+----------------------+                      v
|   BOSMediaStream     |         +---------------------------+
| (64KB Read Cache)    |         |     BOSMediaScheduler     |
+----------+-----------+         | - Deadline Evaluation     |
           |                     | - Pacing (Early/Due/Late) |
           v                     | - Safe Frame Dropping     |
+----------------------+         | - Queue Health Tracking   |
|     MP4 Demuxer      |         +-------------+-------------+
| (O(1) Sample Lookup) |                       |
+----------+-----------+                       |
           |                                   |
           v                                   v
+----------------------+         +---------------------------+
| Bounded Packet Queue | ------> |     Hantro G1 Decoder     |
|   (Cap: 8 Packets)   |         |   (CABAC / Deblocking)    |
+----------------------+         +-------------+-------------+
                                               |
                                               v
                                 +---------------------------+
                                 | Decoded Frame Queue (DFQ) |
                                 |     (Cap: 4 Frames)       |
                                 +-------------+-------------+
                                               |
                                               v
                                 +---------------------------+
                                 | BOSpectra SIMD Dispatcher |
                                 |  - CPUID (SSE2 / AVX2)    |
                                 |  - Streaming Stores       |
                                 |  - Direct DPB -> BOSurface|
                                 +-------------+-------------+
                                               |
                                               v
                                 +---------------------------+
                                 |   BOSurface Window Canvas |
                                 |   (Mapped Ring-3 Memory)  |
                                 +---------------------------+
```

---

## 2. Component Design & Abstractions

### 2.1 Monotonic Media Clock (`BOSMediaClock`)
- **Base Primitives:** `SYS_UPTIME` (syscall 4U) and x86_64 `rdtsc` instruction.
- **Timing Equations:**
  $$\text{media\_time\_us} = (\text{now\_us} - \text{start\_time\_us}) \times \text{playback\_speed}$$
- **A/V Observation:**
  The clock tracks `audio_clock_us` emitted by `BOSAudioStream` via DMA byte position to monitor A/V phase drift without manipulating audio hardware.

### 2.2 Presentation Deadline & Pacing State Machine
For every decoded video frame ready in the Decoded Frame Queue:
$$\Delta_{\text{deadline}} = \text{frame\_pts\_us} - \text{media\_time\_us}$$

| Deadline Offset ($\Delta_{\text{deadline}}$) | State | Engine Action |
|:---|:---:|:---|
| $\Delta_{\text{deadline}} > +10,000 \mu\text{s}$ | **EARLY** | Frame remains in queue. Engine yields CPU slice via `SYS_YIELD`. |
| $-10,000 \mu\text{s} \le \Delta_{\text{deadline}} \le +10,000 \mu\text{s}$ | **READY / DUE** | Frame is presented immediately to BOSurface canvas. |
| $-50,000 \mu\text{s} \le \Delta_{\text{deadline}} < -10,000 \mu\text{s}$ | **LATE** | Frame is presented immediately; lateness telemetry logged; decode backlog increased. |
| $\Delta_{\text{deadline}} < -50,000 \mu\text{s}$ | **SEVERELY_LATE** | Evaluates Safe Drop Policy. If safe, frame is discarded without display. |

### 2.3 Safe Frame Drop Policy
1. **Reference Frame Safety:** The Hantro G1 Decoded Picture Buffer (DPB) internally tracks whether a decoded frame is marked as a short-term/long-term reference picture.
2. **Display Queue Decoupling:** Frame dropping occurs **only** from the `DecodedFrameQueue` (presentation queue), **never** from the DPB. Reference pictures in DPB are retained for motion-compensated prediction of subsequent P/B frames.
3. **Telemetry Enforcement:**
   `[MEDIA-P4] FRAME_DROP: pts=... reason=SEVERELY_LATE ref_safe=1`

### 2.4 Bounded Queue Architecture
- **Packet Queue (PQ):** Capacity = 8 compressed video packets (approx. 200–400 KB).
- **Decoded Frame Queue (DFQ):** Capacity = 4 uncompressed YUV frames (metadata + DPB picture pointers).
- **Backpressure Mechanism:**
  - If DFQ is full ($\ge 4$ frames): Decoder suspends decoding and yields.
  - If PQ is full ($\ge 8$ packets): Demuxer pauses VFS streaming.
  - No unbounded memory growth under CPU stall conditions.

### 2.5 BOSpectra SIMD Dispatcher
Dynamic runtime CPUID-based SIMD vectorization for YUV420P $\to$ ARGB32 color conversion:

```
                  +---------------------------+
                  | BOSpectra SIMD Dispatcher |
                  +-------------+-------------+
                                |
             +------------------+------------------+
             |                                     |
             v                                     v
       [CPUID: AVX2?]                        [CPUID: SSE2?]
             |                                     |
    +--------+--------+                   +--------+--------+
    | YES             | NO                | YES             | NO
    v                 v                   v                 v
[AVX2 Engine]    [Check SSE2]        [SSE2 Engine]    [Scalar Fallback]
(16 px / vector)                     (8 px / vector)  (1 px / vector)
```

1. **Scalar Reference Path:**
   - 100% compliant BT.709 integer formula with saturating clamping.
   - Retained permanently for unit testing, CRC validation, and fallback on legacy CPUs.
2. **SSE2 Vector Path (x86_64 Baseline):**
   - Processes 8 pixels per iteration using 128-bit XMM registers.
   - Integer fixed-point arithmetic (`_mm_madd_epi16`, `_mm_packs_epi32`, `_mm_packus_epi16`).
   - Bit-exact matching with scalar reference path within $\pm 1$ LSB rounding tolerance.
3. **AVX2 Vector Path (Advanced Haswell / Raptor Lake):**
   - Processes 16 pixels per iteration using 256-bit YMM registers.
   - Fused parallel chroma upsampling and color matrix multiplication.
4. **Streaming Direct Stores:**
   - Employs non-temporal vector stores (`_mm_stream_si128` / `_mm256_stream_si256`) when writing full rows into mapped BOSurface memory.
   - Eliminates write-allocate cache fills, preventing video frame data from evicting L1/L2 caches.

### 2.6 Low-Copy / Single-Copy Data Path
1. **Copy Elimination 1 (Packet):** Demuxer reads directly from `BOSMediaStream` 64 KB cache into `sample_buffer` when packet fits in cache, bypassing intermediate OS buffers.
2. **Copy Elimination 2 (Display):** SIMD converter reads directly from Hantro DPB YUV frame buffers and writes directly into mapped `target_fb` BOSurface memory. No intermediate RGB frame buffer.

### 2.7 Algorithmic Demuxer Caching ($O(N) \to O(1)$)
- Add sequential chunk/sample index cursor (`last_sample_idx`, `last_chunk_idx`, `last_file_offset`) inside `MP4Track`.
- During standard forward playback, sample $N+1$ offset is computed in $O(1)$ constant time from sample $N$.

---

## 3. Data & Buffer Ownership Lifecycle

| Phase | Object | Owner | State | Permitted Operations |
|:---|:---|:---|:---|:---|
| 1 | Compressed Packet | `MP4Demuxer` | `DEMUX_OWNED` | Read from VFS, Annex-B start code rewrite |
| 2 | Compressed Packet | `PacketQueue` | `QUEUE_OWNED` | FIFO staging |
| 3 | Compressed Packet | `H264Decoder` | `DECODER_OWNED` | Slice parse, CABAC decode |
| 4 | Decoded Picture | `H264Decoder` (DPB) | `DPB_OWNED` | Motion reference, reconstruction |
| 5 | Frame Pointer | `DecodedFrameQueue`| `QUEUE_OWNED` | Deadline evaluation, queue pacing |
| 6 | Converted Pixels | `BOSpectra SIMD` | `SIMD_OWNED` | Vectorized YUV $\to$ ARGB streaming store |
| 7 | Surface Pixels | `BOSurface` | `SURFACE_OWNED` | Window dirty mark, BCM presentation |
| 8 | Frame Release | `BOSMediaEngine` | `RELEASED` | DPB reference count decremented |

---

## 4. Architectural Telemetry Schema

Phase 4 introduces mandatory `[MEDIA-P4]` telemetry markers:

| Marker | Data Fields | Frequency |
|:---|:---|:---|
| `[MEDIA-P4] ENGINE_START` | `scheduler=monotonic` | Once at pipeline start |
| `[MEDIA-P4] CPU_FEATURES` | `sse2=<0/1> avx2=<0/1>` | Once at feature probe |
| `[MEDIA-P4] SIMD_BACKEND` | `SCALAR / SSE2 / AVX2` | Once at backend activation |
| `[MEDIA-P4] VIDEO_QUEUE_DEPTH` | `depth=<int> max=<int>` | Periodic / on change |
| `[MEDIA-P4] FRAME_PTS` | `pts_us=<int>` | Every presented frame |
| `[MEDIA-P4] MEDIA_TIME` | `time_us=<int>` | Every presented frame |
| `[MEDIA-P4] FRAME_DEADLINE` | `delta_us=<int>` | Every presented frame |
| `[MEDIA-P4] FRAME_STATE` | `EARLY / READY / DUE / LATE` | Every frame evaluation |
| `[MEDIA-P4] FRAME_DROP` | `pts=<int> reason=<str> ref_safe=1`| On dropped frame |
| `[MEDIA-P4] CONVERT_TIME` | `time_us=<int>` | Periodic / benchmark pass |
| `[MEDIA-P4] SIMD_SPEEDUP` | `ratio=<float>` | Benchmark pass |
| `[MEDIA-P4] COPY_ELIMINATED` | `stage=<str> bytes=<int>` | Pipeline init |
