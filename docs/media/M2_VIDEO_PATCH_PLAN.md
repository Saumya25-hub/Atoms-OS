# ATOMS OS — Phase M2: Real MP4 Video Playback Patch Plan
**Protocol Stage**: Task 2 (Architect Team)  
**Date**: September 10, 2026  
**Architect**: ATOMS OS Media & Core Engineering  
**Status**: ARCHITECT PLAN COMPLETE — AWAITING USER APPROVAL

---

## 1. Objective

Transition the ATOMS OS video subsystem from simulated placeholder packets and synthetic color gradients to **genuine, end-to-end MP4 container demuxing and H.264/AVC baseline video decoding** rendered on `BOSurface v2.5` and the physical display.

---

## 2. Hard Requirements & Guardrails

1. **Phase Isolation (Rule 0)**: No code may be edited until this plan is reviewed and approved by the user.
2. **Zero Fake/Generated Frames**: All decoded pixel values must originate from genuine H.264 slice decoding of bitstream payloads extracted from `mdat`.
3. **Strict License Compliance**:
   - Zero GPL / LGPL code.
   - Only Apache-2.0 (`h264bsd`) and CC0-1.0 (`minimp4`) isolated under `third_party/media/`.
4. **Preserve Native BOS Architecture**:
   - Native integration lives exclusively in `kernel/media/`, `kernel/video/`, and `BOSurface` APIs.
   - Do not create a secondary GUI framework or rewrite existing compositor/BWE code.
5. **Freestanding Safety**:
   - Compiles with `-target x86_64-pc-none-elf -msoft-float -mno-sse -mno-sse2 -ffreestanding -mno-red-zone`.
   - Memory allocation routed through `bospectra_mem_alloc` / `bospectra_mem_free`.
6. **Telemetry Verification**:
   Must output the exact expected diagnostic logs on serial/console:
   ```text
   [MP4] file opened
   [MP4] moov parsed
   [MP4] video track found
   [MP4] codec = avc1
   [MP4] resolution = ...
   [MP4] samples = ...
   [MP4] mdat sample extraction = OK
   [H264] SPS = OK
   [H264] PPS = OK
   [H264] decoder initialized
   [VIDEO] frame 0 decoded
   [VIDEO] frame 1 decoded
   ...
   [BOSURFACE] frame presented
   [VIDEO] playback started
   ```
7. **Audio Status**:
   If AAC audio is present in the MP4 container, truthfully report:
   `VIDEO PASS / AUDIO NOT YET SUPPORTED`
   without faking audio buffers.

---

## 3. Detailed Component Modification List

### 3.1 Third-Party Isolation Layer (`third_party/media/`)
- **Action**: Create new isolated directory.
- **Files**:
  - `third_party/media/h264/`: Standalone integer-only C99 H.264 Baseline decoder (`h264bsd`, Apache-2.0). Implements NAL parsing, Exp-Golomb decoding, CAVLC entropy decoding, Intra/Inter macroblock prediction, inverse quantization/transform, and deblocking filter. Allocators wrapped to `bospectra_mem_alloc` and `bospectra_mem_free`.
  - `third_party/media/mp4/`: Lightweight MP4 demuxer helper (`minimp4`, CC0-1.0). Provides sample offset/size calculation routines based on ISO BMFF `stsc`, `stco`, `stsz`, and `avcC` parsing.
  - `third_party/media/LICENSE`: Documenting Apache-2.0 and CC0-1.0 licenses.
- **Why**: Eliminates reinvention of complex mathematical video decoding algorithms while preserving strict license hygiene and architectural isolation.
- **Expected Result**: Clean freestanding compilation without standard C library or floating point dependencies.

### 3.2 MP4 Container Demuxer (`kernel/media/bospectra/container/mp4/`)
- **Files**: `mp4_parser.c`, `mp4_parser.h`
- **Modifications**:
  - Extend `MP4_TrackContext` to store:
    - Sample-to-Chunk table (`stsc`)
    - Chunk Offset table (`stco` / `co64`)
    - Sample Size table (`stsz`)
    - Time-to-Sample table (`stts`)
    - Sync Sample table (`stss` keyframes)
    - Decoder Configuration Record (`avcC` with SPS/PPS NAL units)
  - Replace `mp4_read_packet`:
    - Given sample index $i$, lookup corresponding chunk $c$ via `stsc`.
    - Retrieve chunk base offset in file via `stco`/`co64`.
    - Sum sizes of preceding samples in chunk $c$ using `stsz` to determine exact file offset in `mdat`.
    - Allocate `BOSPacket` with exact sample size.
    - Read payload directly from file using `bospectra_file_seek` and `bospectra_file_read`.
    - Assign true PTS/DTS from `stts` and keyframe flag from `stss`.
  - Add telemetry messages: `[MP4] file opened`, `[MP4] moov parsed`, `[MP4] video track found`, `[MP4] codec = avc1`, `[MP4] resolution = ...`, `[MP4] samples = ...`, `[MP4] mdat sample extraction = OK`.
- **Why**: Eliminates the 4096-byte dummy packet generator and delivers genuine compressed H.264 sample packets to the decoder.

### 3.3 H.264 Video Decoder (`kernel/media/bospectra/decoder/h264/`)
- **Files**: `h264_decoder.c`, `h264_decoder.h`
- **Modifications**:
  - Replace fake modulo gradient generation with `h264bsd` decoder pipeline:
    - In `h264_open`: Allocate decoder instance; initialize internal state.
    - Parse extradata (`avcC` containing SPS & PPS NAL units) and submit to decoder.
    - In `h264_decode_packet`:
      - Unpack AVCC 4-byte length-prefixed NAL units from `packet->data`.
      - Invoke decoder for each NAL unit (SPS, PPS, Slice).
      - When picture decoding completes, acquire `BOSFrame` (YUV420P) from `bospectra_frame_acquire`.
      - Copy decoded planar Y, U, and V macroblock pixels into `frame->data[0..2]`.
      - Set `frame->pts`, `frame->dts`, `frame->duration_us`.
  - Add telemetry messages: `[H264] SPS = OK`, `[H264] PPS = OK`, `[H264] decoder initialized`, `[VIDEO] frame %u decoded`.
- **Why**: Delivers real video frames to the compositor.

### 3.4 Media Presentation & BOSurface (`kernel/media/bospectra/render/backends/software/`)
- **Files**: `software_backend.c`
- **Modifications**:
  - Add aspect-ratio preservation calculation (letterbox/pillarbox) when blitting to viewport.
  - Emit presentation telemetry: `[BOSURFACE] frame presented`, `[VIDEO] playback started`.
- **Why**: Fulfills the BOSurface v2.5 presentation requirements without creating redundant GUI layers.

### 3.5 Disk Image & Player Launch (`tools/image_builder.c`, `kernel/shell/apps/bos_media_player/`)
- **Files**: `tools/image_builder.c`, `bos_media_player.c`
- **Modifications**:
  - In `image_builder.c`: Add `TEST.MP4` to FAT32 root directory table and data clusters.
  - In `bos_media_player.c`: Prioritize opening `TEST.MP4`, `/TEST.MP4`, `/Media/TEST.MP4`.
- **Why**: Ensures the test video is present on the storage medium and launched during media player startup.

### 3.6 Build System (`build.ps1`)
- **Files**: `build.ps1`
- **Modifications**:
  - Add compile rules for `third_party/media/h264/*.c` and `third_party/media/mp4/*.c`.
  - Append objects to `link_response.txt`.
- **Why**: Integrates the new freestanding decoders into the kernel link stage cleanly.

---

## 4. Verification Plan

1. **Compilation**:
   Run `build.ps1` to ensure zero compilation or linking errors with strict freestanding flags.
2. **QEMU Pure UEFI Pre-Flight**:
   Run `tools/verify_phase_m2_video.py` in QEMU:
   - Verify pure UEFI boot.
   - Verify ABDE diagnostic table renders.
   - Verify heartbeat spinner rotates (`| / - \`).
   - Verify exact telemetry sequence:
     ```text
     [MP4] file opened
     [MP4] moov parsed
     [MP4] video track found
     [MP4] codec = avc1
     [MP4] resolution = 640x360
     [MP4] samples = ...
     [MP4] mdat sample extraction = OK
     [H264] SPS = OK
     [H264] PPS = OK
     [H264] decoder initialized
     [VIDEO] frame 0 decoded
     [VIDEO] frame 1 decoded
     [BOSURFACE] frame presented
     [VIDEO] playback started
     ```
   - Verify AAC audio status: `VIDEO PASS / AUDIO NOT YET SUPPORTED`.
   - Verify no regression in Phase M1 Intel HDA audio foundation.
3. **Bare-Metal Haswell H81 Validation**:
   Validate over PXE network boot (`task-3186`) on physical hardware.

---

## 5. Rollback Plan

If decoding regression or instability occurs:
1. `git checkout kernel/media/bospectra/container/mp4/`
2. `git checkout kernel/media/bospectra/decoder/h264/`
3. `git checkout tools/image_builder.c`
4. Recompile and restore Phase M1 certified state.
