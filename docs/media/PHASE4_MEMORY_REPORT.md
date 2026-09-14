# ATOMS OS — Phase 4 Memory Architecture Report
## Bounded Buffers & Low-Copy Pipeline Audit

**Date**: 2026-09-12  
**Subsystem**: Native Media Engine Memory Lifecycle  

---

## 1. Memory Copy Elimination Audit

Prior to Phase 4, presenting a single $1920 \times 1080$ frame required 5 separate memory copies. Phase 4 achieved direct single-copy delivery:

```
[VFS Storage]
      |
      | (Chunked DMA / VFS cache)
      v
[BOSMediaStream 64KB Cache]
      |
      | (Zero-copy parse / Direct buffer reference)
      v
[MP4 Demuxer]
      |
      | (Single copy into decoder sample buffer)
      v
[Hantro G1 H.264 DPB] (Decoded YUV420P Picture)
      |
      |  *** COPY ELIMINATED: DIRECT SIMD CONVERSION ***
      |  (Zero intermediate RGB buffer; writes directly to mapped BOSurface)
      v
[BOSurface Mapped ARGB32 Framebuffer]
      |
      | (Hardware Page Flipping / GOP Scanout)
      v
[Physical Display]
```

### Measured Eliminations:
1. **Intermediate Framebuffer Copy:**
   - **Stage:** `DPB_TO_SURFACE`
   - **Bytes Saved per 1080p Frame:** $1920 \times 1080 \times 4 = 8,294,400\text{ bytes}$ (8.29 MB)
   - **Telemetry:** `[MEDIA-P4] COPY_ELIMINATED: stage=DPB_TO_SURFACE bytes=8294400`
2. **Contiguous Demux Buffer Copy:**
   - **Stage:** `VFS_CACHE_TO_DEMUX`
   - **Bytes Saved per Stream Buffer:** $131,072\text{ bytes}$ (128 KB)
   - **Telemetry:** `[MEDIA-P4] COPY_ELIMINATED: stage=VFS_CACHE_TO_DEMUX bytes=131072`

---

## 2. Bounded Queue Dimensions & Watermarks

To prevent unbounded dynamic memory expansion under slow decode or high CPU load, all pipeline queues operate with strict bounds:

| Queue | Entity Managed | Capacity | High-Water Mark | Low-Water Mark | Storage Type |
|:---|:---|:---:|:---:|:---:|:---|
| **Packet Queue (PQ)** | Compressed NAL packets | 8 packets | 4 packets | 1 packet | Static `.bss` buffer |
| **Decoded Frame Queue (DFQ)** | Uncompressed YUV pictures | 4 frames | 2 frames | 0 frames | DPB pointer table |
| **Audio FIFO** | S16_LE PCM audio | 64 KB | 32 KB | 4 KB | Ring buffer |
| **BOSurface Pool** | Double-buffered window surfaces | 2 surfaces | 2 surfaces | 1 surface | Mapped Ring-3 VMM |

---

## 3. ELF Memory Layout & Stack Guard Headroom

Freestanding userspace binaries in ATOMS OS are bounded by the VMM stack guard page at `0x400FB000`. Memory inspection confirms complete structural safety:

```
0x40000000 ┌───────────────────────────────────────────────┐
           │ .text (Code Segment, 267 KB)                  │
0x40042000 ├───────────────────────────────────────────────┤
           │ .rodata + .eh_frame (Consts/Tables, 68 KB)    │
0x40054000 ├───────────────────────────────────────────────┤
           │ .data (Initialized Globals, 2 KB)             │
0x40055000 ├───────────────────────────────────────────────┤
           │ .bss (Queues, Buffers, Streams, 535 KB)       │
0x400DAC94 └───────────────────────────────────────────────┘
           │                                               │
           │  === 131,948 BYTES (128.8 KB) HEADROOM ===    │
           │                                               │
0x400FB000 ┌───────────────────────────────────────────────┐
           │ VMM STACK GUARD PAGE (FAULT TRIGGER)          │
0x400FC000 ├───────────────────────────────────────────────┤
           │ Process User Stack (Grows Downward)           │
0x40100000 └───────────────────────────────────────────────┘
```

**Verdict:** Zero collision with stack guard page; 128.8 KB of clear headroom guaranteed.
