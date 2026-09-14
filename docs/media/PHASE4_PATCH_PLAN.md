# ATOMS OS — Phase 4 Patch Plan
**Subsystem:** Native Media Engine ➔ Time-Aware SIMD & Low-Copy Video Pipeline  
**Milestone:** Phase 4C (Implementation & Risk Plan)  
**Date:** September 12, 2026  
**Status:** **APPROVED FOR EXECUTION (RULE 0 ENFORCED — NO CODE MODIFIED)**  

---

## 1. Scope of Work

Phase 4 implements the time-aware, SIMD-accelerated, bounded media execution engine specified in `PHASE4_ARCHITECTURE.md`.

### Permitted Files to Modify / Create:
1. `userspace/libbos_media/include/bos_media_simd.h` (NEW: SIMD Dispatcher & vector conversion API)
2. `userspace/libbos_media/src/bos_media_simd.c` (NEW: Scalar reference path, SSE2 128-bit path, AVX2 256-bit path, runtime CPUID detection)
3. `userspace/libbos_media/include/bos_media_clock.h` (NEW: Monotonic media clock & presentation deadline scheduler API)
4. `userspace/libbos_media/src/bos_media_clock.c` (NEW: Media clock timing, frame deadline classification, bounded queues)
5. `userspace/libbos_media/demux/mp4_demuxer.h` (Add sequential sample cursor to `MP4Track`)
6. `userspace/libbos_media/demux/mp4_demuxer.c` (Implement $O(1)$ cached sample lookup)
7. `userspace/libbos_media/src/bos_media_pipeline.h` (Add scheduler, SIMD context, and queue structures)
8. `userspace/libbos_media/src/bos_media_pipeline.cpp` (Integrate SIMD converter, deadline pacing, direct DPB-to-BOSurface writes, and `[MEDIA-P4]` telemetry)
9. `userspace/apps/media_player/main.cpp` (Update main event loop to respect deadline pacing and log Phase 4 metrics)

### Strictly Forbidden Modifications:
- ❌ NO modifications to `kernel/` core files (`kernel.c`, `vmm.c`, `pmm.c`, `scheduler.c`).
- ❌ NO modifications to legacy BCM compositor (`kernel/compositor/`).
- ❌ NO modifications to third-party Hantro G1 decoder internals (`third_party/media/h264/src/`).
- ❌ NO fake SIMD or synthetic time increments.

---

## 2. Step-by-Step Implementation Strategy

### Step 1: SIMD Dispatcher & Color Conversion (`bos_media_simd.*`)
- **What:** Create `BOSpectra SIMD Dispatcher` supporting BT.709 YUV420P $\to$ ARGB32 conversion.
- **Why:** Replace the scalar inner loop currently performing 2M coordinate divisions and 12M branch clamps per frame.
- **Paths:**
  - `yuv420p_to_argb_scalar`: Exact integer reference path.
  - `yuv420p_to_argb_sse2`: 128-bit vector path (8 pixels/iteration) with non-temporal stores.
  - `yuv420p_to_argb_avx2`: 256-bit vector path (16 pixels/iteration).
- **CPUID:** Probes Leaf 1 EDX (bit 26) for SSE2 and Leaf 7 EBX (bit 5) for AVX2.
- **Expected Result:** $6\times$ to $10\times$ speedup in pixel conversion; zero cache pollution.

### Step 2: Monotonic Media Clock & Presentation Scheduler (`bos_media_clock.*`)
- **What:** Create `BOSMediaClock` and `BOSMediaScheduler`.
- **Why:** Replace unpaced execution with deadline-driven frame presentation tied to stream PTS.
- **States:**
  - `EARLY`: Delta $> +10\text{ ms}$ $\to$ yield CPU.
  - `READY / DUE`: $|\text{Delta}| \le 10\text{ ms}$ $\to$ present to BOSurface.
  - `LATE`: $-50\text{ ms} \le \text{Delta} < -10\text{ ms}$ $\to$ present immediately, mark latency.
  - `SEVERELY_LATE`: $\text{Delta} < -50\text{ ms}$ $\to$ trigger Safe Drop Policy.
- **Expected Result:** Stable frame rates (24, 30, 60 FPS) matching media timestamps; zero unbounded A/V drift.

### Step 3: MP4 Demuxer $O(1)$ Cursor Optimization (`mp4_demuxer.*`)
- **What:** Cache `last_chunk_idx`, `last_sample_idx`, and `last_file_offset` in `MP4Track`.
- **Why:** Eliminate $O(N^2)$ linear search from chunk 0 on every sample request.
- **Expected Result:** Constant time $O(1)$ sample location during forward playback.

### Step 4: Pipeline Integration & Direct Surface Writing (`bos_media_pipeline.*`)
- **What:** Wire the SIMD dispatcher, media clock, and bounded frame queues into `bos_media_pipeline_render_frame()`.
- **Why:** Enable direct low-copy pixel writes from Hantro DPB into mapped BOSurface memory with deadline pacing.
- **Expected Result:** Clean presentation loop emitting all `[MEDIA-P4]` telemetry markers.

### Step 5: Media Player GUI Application Update (`main.cpp`)
- **What:** Connect UI loop to deadline-paced pipeline and log Phase 4 startup markers.
- **Why:** Enable responsive keyboard/mouse handling during continuous video playback.

---

## 3. Risk Analysis & Mitigation

| Risk | Likelihood | Impact | Mitigation Strategy |
|:---|:---:|:---:|:---|
| **AVX2 Illegal Instruction on Legacy CPUs** | Low | Critical | Runtime CPUID validation. If CPU lacks AVX2, dispatch table automatically falls back to SSE2 or Scalar. |
| **SIMD Output Pixel Discrepancy** | Medium | High | Unit test comparing scalar output against SSE2/AVX2 pixel-by-pixel with CRC verification. |
| **Stack Overflow in Freestanding Userspace** | Low | High | Bounded queues allocate memory statically in `.bss`; total `.bss` footprint strictly monitored below stack guard page (`0x400FB000`). |
| **Audio Desync from Video Pacing** | Low | Medium | Media clock observes audio DMA position to calibrate master timebase without mutating audio hardware. |

---

## 4. Rollback Plan

1. All new Phase 4 modules (`bos_media_simd.*`, `bos_media_clock.*`) are self-contained.
2. If SIMD exhibits artifacts, setting `SIMD_FORCE_SCALAR=1` forces the proven scalar reference path without rebuilding the application.
3. Git working tree permits reverting `bos_media_pipeline.cpp` and `main.cpp` back to certified Phase 3 baseline (`git checkout`) in under 5 seconds.
