# ROAD 2 — H.264 Decoder Candidate Analysis

> **Purpose**: Select ONE mature, proven, license-compatible H.264 High Profile decoder to replace the incomplete Hantro G1 decoder in the ATOMS OS Ring-3 Media Player.

---

## Target File Requirements

| Property | Value |
|----------|-------|
| File | `Dolby_Vision_AtmosHDR.mp4` |
| Codec | H.264 / AVC |
| Profile | **High (IDC 100)** |
| Level | 4.0 |
| Entropy | **CABAC** |
| Transform | **8×8 enabled** (`transform_8x8_mode_flag = 1`) |
| Resolution | 1920 × 1080 |
| Frame Type | I + P + B slices |

**All 10 required capabilities**:
1. H.264 High Profile
2. CABAC entropy decoding
3. 8×8 inverse transform (DCT)
4. 8×8 intra prediction
5. Scaling matrices
6. CAVLC (where applicable)
7. Inter prediction (P/B slices, quarter-pel motion compensation)
8. Reference picture / DPB handling
9. Real frame reconstruction (no fake/concealment frames)
10. Freestanding or easily adaptable C implementation

---

## Candidate Summary Table

| Criteria | A: AOSP `libavc` | B: Cisco `openh264` | C: FFmpeg `libavcodec` |
|----------|:-----------------:|:-------------------:|:----------------------:|
| High Profile | ✅ YES | ❌ NO (Baseline only decoder) | ✅ YES |
| CABAC | ✅ YES | ❌ NO | ✅ YES |
| 8×8 Transform | ✅ YES | ❌ NO | ✅ YES |
| 8×8 Intra Pred | ✅ YES | ❌ NO | ✅ YES |
| Scaling Matrices | ✅ YES | ❌ NO | ✅ YES |
| CAVLC | ✅ YES | ✅ YES | ✅ YES |
| Inter Prediction | ✅ YES | ✅ YES | ✅ YES |
| DPB Management | ✅ YES | ✅ YES | ✅ YES |
| License | **Apache-2.0** | **BSD-2-Clause** | **LGPL-2.1+** |
| Freestanding? | ✅ Adaptable | ⚠️ Needs POSIX stubs | ❌ Heavy libc deps |
| File Count (decoder) | ~65 C files | ~120 C++ files | ~50+ tightly coupled |
| Integration Complexity | **Medium** | N/A (rejected) | **Very High** |
| **VERDICT** | **✅ RECOMMENDED** | **❌ REJECTED** | **⚠️ NOT RECOMMENDED** |

---

## Candidate A: AOSP `libavc` (RECOMMENDED)

### Identity
- **Project**: Android Open Source Project `external/libavc`
- **Upstream**: `https://android.googlesource.com/platform/external/libavc`
- **Branch**: `refs/heads/main`
- **Original Developer**: Ittiam Systems Pvt. Ltd, Bangalore (contributed to AOSP)
- **Production Use**: Android software H.264 decoder since Android 5.0+ (used by `C2SoftAvcDec` / Codec2 framework)

### License Audit

**Apache License 2.0** — Permissive, GPL-compatible, no copyleft obligations.

Every source file in `decoder/` and `common/` contains the identical Apache-2.0 header:
```
Copyright (C) 2015 The Android Open Source Project
Licensed under the Apache License, Version 2.0
Originally developed and contributed by Ittiam Systems Pvt. Ltd, Bangalore
```

**ATOMS compatibility**: Full. Apache-2.0 allows static linking, modification, redistribution with attribution. No source disclosure requirement. Only obligation: include NOTICE file.

### Verified Capabilities (from source inspection)

| Feature | Evidence |
|---------|----------|
| CABAC | `ih264d_cabac.c`, `ih264d_parse_cabac.c`, `ih264d_cabac_init_tables.c`, defines `LUMA_8X8_CTXCAT = 5` |
| CAVLC | `ih264d_parse_cavlc.c`, `ih264_cavlc_tables.c` |
| 8×8 Transform | `ih264_iquant_itrans_recon.c` (contains 4×4 AND 8×8 inverse transform paths) |
| 8×8 Intra Pred | `ih264_luma_intra_pred_filters.c` (contains 4×4, 8×8, and 16×16 modes) |
| Scaling Matrices | `ih264d_quant_scaling.c` / `.h` (High Profile quant scaling lists) |
| Inter Prediction | `ih264d_inter_pred.c`, `ih264_inter_pred_filters.c` (quarter-pel MC) |
| DPB Management | `ih264d_dpb_mgr.c`, `ih264_dpb_mgr.c`, `ih264_buf_mgr.c` |
| Deblocking | `ih264d_deblocking.c`, `ih264_deblk_edge_filters.c` |
| I/P/B Slices | `ih264d_parse_islice.c`, `ih264d_parse_pslice.c`, `ih264d_parse_bslice.c` |
| NAL Parsing | `ih264d_nal.c` |
| SPS/PPS/Slice Header | `ih264d_parse_headers.c`, `ih264d_parse_slice.c` |
| SEI/VUI | `ih264d_sei.c`, `ih264d_vui.c` |
| Format Conversion | `ih264d_format_conv.c` (YUV420P output) |

### Files Required (decoder core)

**`decoder/` directory** (~32 .c + ~33 .h = ~65 files):
```
ih264d_api.c              ih264d_bitstrm.c          ih264d_cabac.c
ih264d_cabac_init_tables.c ih264d_compute_bs.c      ih264d_deblocking.c
ih264d_dpb_mgr.c          ih264d_format_conv.c      ih264d_function_selector_generic.c
ih264d_inter_pred.c       ih264d_mb_utils.c         ih264d_mvpred.c
ih264d_nal.c              ih264d_parse_bslice.c     ih264d_parse_cabac.c
ih264d_parse_cavlc.c      ih264d_parse_headers.c    ih264d_parse_islice.c
ih264d_parse_mb_header.c  ih264d_parse_pslice.c     ih264d_parse_slice.c
ih264d_process_bslice.c   ih264d_process_intra_mb.c ih264d_process_pslice.c
ih264d_quant_scaling.c    ih264d_sei.c              ih264d_tables.c
ih264d_thread_compute_bs.c ih264d_thread_parse_decode.c ih264d_utils.c
ih264d_vui.c
+ all corresponding .h headers
```

**`common/` directory** (~20 .c + ~25 .h = ~45 files):
```
ih264_buf_mgr.c           ih264_cabac_tables.c      ih264_cavlc_tables.c
ih264_chroma_intra_pred_filters.c  ih264_common_tables.c  ih264_deblk_edge_filters.c
ih264_deblk_tables.c      ih264_disp_mgr.c          ih264_dpb_mgr.c
ih264_ihadamard_scaling.c ih264_inter_pred_filters.c ih264_iquant_itrans_recon.c
ih264_list.c              ih264_luma_intra_pred_filters.c  ih264_mem_fns.c
ih264_padding.c           ih264_resi_trans_quant.c   ih264_trans_data.c
ih264_weighted_pred.c     ithread.c
+ all corresponding .h headers
```

### Dependencies

| Dependency | Status | Adaptation Strategy |
|-----------|--------|---------------------|
| `<string.h>` (memcpy, memset) | Used in common/ | Already available in ATOMS freestanding runtime |
| `<stdlib.h>` (malloc/free) | Used in ih264d_api.c | Replace with ATOMS `u_malloc`/`u_free` Ring-3 heap |
| `ithread.c` (pthread wrappers) | Threading abstraction | Stub to single-threaded (disable `ih264d_thread_*`) |
| x86 SIMD (`decoder/x86/`) | Optional SSE4.2 intrinsics | Skip initially — use `generic` function selector |
| ARM NEON (`decoder/arm/`) | ARM SIMD | Not needed — skip |

### Integration Architecture

```
MP4 Demux → NAL Units → libavc API (ih264d_video_decode) → YUV420P frame
                                                              ↓
Ring-3 Media Player → bos_media_simd (SSE2 YUV→ARGB) → BOSurface framebuffer
```

**API is clean** — `ih264d_api.c` exposes a simple call-based interface:
1. `ih264d_create()` — Allocate decoder
2. `ih264d_init()` — Initialize
3. `ih264d_video_decode()` — Decode one NAL unit, get YUV output
4. `ih264d_delete()` — Cleanup

### Integration Complexity: MEDIUM

| Task | Effort |
|------|--------|
| Copy ~100 files to `third_party/media/libavc/` | Low |
| Create `build/compile_libavc.ps1` | Low |
| Stub `ithread.c` for single-threaded | Low |
| Replace `malloc/free` with ATOMS Ring-3 heap | Low |
| Remove `printf/fprintf` for freestanding | Low (same as Hantro G1 cleanup) |
| Wire NAL feeder: MP4 sample → `ih264d_video_decode()` | Medium |
| Wire YUV output to `bos_media_simd_yuv420p_to_argb()` | Low (already exists) |
| **Total estimated new/modified lines** | **~200-300 lines** of integration glue |

### Risks

1. **Memory footprint**: Full DPB for 1080p Level 4.0 requires ~32 MB (16 ref frames × 1920×1080 × 1.5). Must verify Ring-3 heap capacity.
2. **First-time performance**: Without SIMD, software decode at 1080p will be slow (~2-5 FPS on Haswell i3). Acceptable for milestone certification.
3. **Threading stubs**: Disabling multi-threading may need careful null-checks in thread_compute_bs paths.

---

## Candidate B: Cisco OpenH264 (REJECTED)

### Identity
- **Project**: `github.com/cisco/openh264`
- **License**: **BSD-2-Clause** (very permissive)

### Why Rejected

OpenH264's **decoder** only supports **Constrained Baseline Profile**.

From OpenH264 documentation and source:
- The **encoder** supports Baseline/Main/High Profile
- The **decoder** supports only Constrained Baseline Profile (no CABAC, no 8×8 transform, no B-slices)

| Required Feature | OpenH264 Decoder |
|-----------------|-----------------|
| High Profile | ❌ NO |
| CABAC | ❌ NO |
| 8×8 Transform | ❌ NO |
| B-slices | ❌ NO |

**Verdict**: Cannot decode `Dolby_Vision_AtmosHDR.mp4`. Same limitation as Hantro G1.

---

## Candidate C: FFmpeg `libavcodec` (NOT RECOMMENDED)

### Identity
- **Project**: FFmpeg
- **License**: **LGPL-2.1+** (with some files GPL-2.0+)

### Capabilities
Complete H.264 decoder — all profiles up to High 4:4:4 Predictive. Reference-grade, battle-tested.

### Why Not Recommended

Integration complexity is prohibitively high for a freestanding OS kernel.

1. **Massive dependency graph**: `h264dec.c` pulls in `h264_cabac.c`, `h264_cavlc.c`, `h264_slice.c`, `h264_mb.c`, `h264_loopfilter.c`, `h264_refs.c`, `h264_direct.c`, `h264_ps.c`, etc. — each depending on `avcodec.h`, `internal.h`, `codec_internal.h`, `get_bits.h`, `cabac.h`, etc.
2. **libc dependencies**: `malloc`, `realloc`, `free`, `memcpy`, `memset`, `log2f`, `av_log`, `av_malloc`, `av_freep` — requires full `libavutil` which itself has 100+ files.
3. **LGPL-2.1 compliance**: Requires either dynamic linking (not feasible in Ring-3 freestanding) or full source availability alongside ATOMS. Manageable but adds legal overhead.
4. **File count**: Minimum ~50 source files just for H.264 decoder + ~30 for libavutil core = ~80 files with complex interdependencies.
5. **Not designed for freestanding**: Assumes POSIX-complete userland with `stdio.h`, `math.h`, threading, dynamic memory.

---

## Final Recommendation

**CANDIDATE A: AOSP `libavc`** is the clear winner.

### Decision Matrix

| Factor | A: libavc | B: OpenH264 | C: FFmpeg |
|--------|:---------:|:-----------:|:---------:|
| Meets ALL 10 requirements | ✅ | ❌ | ✅ |
| License compatibility | ✅ Apache-2.0 | ✅ BSD-2 | ⚠️ LGPL-2.1 |
| Integration effort | Medium | N/A | Very High |
| Production proven | ✅ (Android 5.0+) | ✅ | ✅ |
| Freestanding adaptable | ✅ | N/A | ❌ |
| Risk level | Low-Medium | N/A | High |

### Recommended Next Steps (after user approval)

1. Clone/download `external/libavc` from AOSP (`decoder/` + `common/` directories only)
2. Place in `third_party/media/libavc/`
3. Create `build/compile_libavc.ps1` — compile ~100 C files with freestanding flags
4. Stub `ithread.c` for single-threaded operation
5. Remove `printf`/`fprintf` calls for freestanding
6. Replace `malloc`/`free` with ATOMS Ring-3 heap (`u_malloc`/`u_free`)
7. Wire MP4 demux NAL output → `ih264d_video_decode()` → YUV420P → existing `bos_media_simd` → BOSurface
8. Build and test with `Dolby_Vision_AtmosHDR.mp4`

---

> **STOP**: This document is the analysis deliverable. No implementation has been started. Awaiting user decision before any code changes.
