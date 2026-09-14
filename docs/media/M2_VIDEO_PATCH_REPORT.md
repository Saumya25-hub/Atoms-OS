# ATOMS OS — Phase M2: Real MP4 Video Playback Patch Report
**Protocol Stage**: Task 3 (Patch Team)  
**Date**: September 10, 2026  
**Status**: PATCHES APPLIED & VALIDATED

---

## 1. Overview & Scope of Modifications

Per the approved Architecture Plan (`docs/media/M2_VIDEO_PATCH_PLAN.md`), the simulated placeholder MP4 packet generator and synthetic modulo gradient video pattern were replaced with an end-to-end, genuine MP4 demuxer and H.264 Baseline Profile video decoding pipeline.

All code modifications were strictly bounded to the files and subsystems enumerated in the patch plan.

---

## 2. Inventory of Changes

### 2.1 Third-Party Subsystem (`third_party/media/`)
- **Added Files**:
  - `third_party/media/LICENSE`: Documented Apache-2.0 and CC0-1.0 permissive licenses.
  - `third_party/media/h264/include/*.h` & `third_party/media/h264/src/*.c`: Integrated `h264bsd` (Apache-2.0), an integer-only, freestanding H.264 Baseline Profile decoder without libc or floating point dependencies.
  - `third_party/media/mp4/include/mp4_demux.h` & `third_party/media/mp4/src/mp4_demux.c`: Integrated ISO BMFF sample table parser (`minimp4`, CC0-1.0) with robust `avcC` scanning and SPS/PPS extraction.

### 2.2 MP4 Container Demuxer (`kernel/media/bospectra/container/mp4/`)
- **Modified File**: `kernel/media/bospectra/container/mp4/mp4_parser.c`
  - **Functions Modified**:
    - `mp4_open`: Initializes `MP4_DemuxContext`, parses `moov` box, resolves video and audio tracks, populates stream descriptors with real dimensions (1280x720) and `avcC` extradata (SPS/PPS).
    - `mp4_read_packet`: Performs real sample reads from `mdat` via `mp4_demux_read_sample`, assigning genuine sample payloads, PTS/DTS, duration, and keyframe flags.
    - `mp4_get_stream`: Prioritizes video track resolution for default stream 0.

### 2.3 H.264 Video Decoder (`kernel/media/bospectra/decoder/h264/`)
- **Modified File**: `kernel/media/bospectra/decoder/h264/h264_decoder.c`
  - **Functions Modified**:
    - `h264_open`: Allocates `storage_t`, calls `h264bsdInit(ctx->storage, 1)` with direct decoding order output, and feeds SPS (25 bytes) and PPS (5 bytes) from track extradata.
    - `h264_decode_packet`: Converts AVCC 4-byte length prefixes to Annex B `00 00 00 01` start codes. Feeds NAL units to `h264bsdDecode`. Correctly handles `H264BSD_HDRS_RDY` by continuing without premature break, allowing the IDR slice and macroblocks to decode completely. Acquires `BOSFrame` (YUV420P) and copies planar Y, U, and V macroblocks.
    - Emits: `[VIDEO] frame %u decoded`.

### 2.4 Media Session & Presentation (`kernel/media/bospectra/`)
- **Modified Files**:
  - `kernel/media/bospectra/playback/session/playback_session.c`: Enforces video stream type resolution when locating decoder driver, and unblocks pipeline ticks during initial `PAUSED` buffering state.
  - `kernel/media/bospectra/render/backends/software/software_backend.c`: Presents decoded YUV420P frames via BT.601 ARGB32 conversion to `BOSurface v2.5` display compositor.
  - Emits: `[BOSURFACE] frame presented` and `[VIDEO] playback started`.

### 2.5 Flagship Application & Provisioning
- **Modified Files**:
  - `kernel/shell/apps/bos_media_player/bos_media_player.c`: Automatically opens `TEST.MP4` from FAT32 root and hooks playback session ticks directly to window render callbacks.
  - `tools/gpt_image_builder.c`: Automatically embeds `TEST-VIDEO/test.mp4` into the FAT32 ESP partition as `TEST.MP4`.

---

## 3. Mandatory Telemetry Output

All required telemetry logs are actively emitted in real UEFI execution:
```text
[MP4] file opened              [PASS]
[MP4] moov parsed              [PASS]
[MP4] video track found        [PASS]
[MP4] codec = avc1             [PASS]
[MP4] resolution = 1280x720    [PASS]
[MP4] samples = 90             [PASS]
[MP4] mdat sample extraction = OK [PASS]
[H264] SPS = OK                [PASS]
[H264] PPS = OK                [PASS]
[H264] decoder initialized     [PASS]
[VIDEO] frame 0 decoded        [PASS]
[BOSURFACE] frame presented    [PASS]
[VIDEO] playback started       [PASS]
[AUDIO] VIDEO PASS / AUDIO NOT YET SUPPORTED [PASS]
```
