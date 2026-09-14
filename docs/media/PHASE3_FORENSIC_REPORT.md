# ATOMS OS — Phase 3 Forensic Audit Report
**Subsystem:** Userspace Media Engine ➔ VFS + Audio Userspace Bridges  
**Milestone:** Phase 3A (Forensic Audit & Component Classification)  
**Date:** September 12, 2026  
**Status:** **AUDIT COMPLETE (RULE 0 ENFORCED — NO CODE MODIFIED)**  

---

## 1. Executive Summary

Phase 2 successfully integrated the real Hantro G1 H.264 core decoding engine, FFmpeg `libavcodec` CABAC tables, and `minimp3` into freestanding Ring-3 userspace. However, the bridges connecting the media engine to ATOMS OS native services (VFS, Audio HAL, memory lifecycles, and process isolation) still contain architectural gaps:
1. **VFS Media Stream Bridge:** Performs synchronous, unbuffered `SYS_READ` syscalls per packet; lacks `eof()` and `error()` methods; uses a single static stream singleton (`s_stream_instance`); lacks robust signed seek validation.
2. **Audio Userspace Bridge:** Lacks the unified `BOSAudioStream` abstraction; lacks format negotiation (hardcoded to 44.1 kHz / S16_LE / 2 channels); lacks a bounded userspace FIFO queue with backpressure; lacks kernel pointer validation for `SYS_AUDIO_CALL` (Op `STREAM_WRITE`).
3. **Process Isolation & Resource Cleanup:** Abrupt process termination reaps tasks and surfaces, but currently leaks audio stream descriptors and VFS file descriptors.
4. **Seeking Semantics:** Video timeline seeking is a stub in `bos_media_pipeline_seek()`, failing to flush the decoder DPB or reposition the demuxer sample index.

---

## 2. Comprehensive Component Classification

Every media subsystem component has been inspected and classified according to the ATOMS OS Phase 3 Specification:

| Component / Subsystem | Source Path | Classification | Current State & Forensic Assessment | Phase 3 Action |
|:---|:---|:---:|:---|:---|
| **Media Player App** | `userspace/apps/media_player/main.cpp` | **PRODUCTION / PARTIAL** | Real Ring-3 GUI application mapping BOSurface v2.5; space/seek keys mapped; seek callback does not yet trigger pipeline flush. | **ENHANCE**: Connect seek keys to `bos_media_pipeline_seek()`, display stream stats. |
| **BOSMediaStream Interface** | `userspace/libbos_media/include/bos_media_stream.h` | **PARTIAL** | Declares `read`, `seek`, `tell`, `close`. Missing `eof`, `error`, `size` method pointers, and buffer configuration. | **EXPAND**: Add `eof()`, `error()`, dynamic instance allocation, and buffering parameters. |
| **VFS Stream Implementation** | `userspace/libbos_media/src/bos_media_stream.c` | **PARTIAL** | Wraps `SYS_OPEN`, `SYS_READ`, `SYS_SEEK`, `SYS_CLOSE`. Single static instance; zero read buffering; raw uninstrumented syscalls. | **REVISE**: Implement multi-instance stream pool, read cache/buffer, and `[MEDIA-P3]` telemetry. |
| **BOSAudioStream Interface** | `userspace/libbos_media/audio/bos_audio_stream.*` | **MISSING** | Does not exist. Code directly invokes raw `audio_stream_*` functions from `audio_user.c`. | **CREATE**: Implement complete `BOSAudioStream` abstraction (`create`, `configure`, `start`, `write`, `pause`, `resume`, `drain`, `flush`, `stop`, `destroy`). |
| **Audio User Syscall Bridge** | `userspace/libs/audio/audio_user.c` | **REAL** | Translates high-level audio functions to `SYS_AUDIO_CALL` (43U) sub-operations. | **REUSE**: Act as low-level kernel syscall bridge underneath `BOSAudioStream`. |
| **Audio Format Negotiation** | `userspace/libbos_media/src/bos_media_pipeline.cpp` | **PARTIAL** | Hardcoded to 44,100 Hz / Stereo / S16_LE. No support for 48 kHz, FLTP/S32 conversion, or mono-to-stereo expansion. | **IMPLEMENT**: Format negotiation state machine and sample conversion. |
| **Audio FIFO / Queue** | `userspace/libbos_media/audio/` | **MISSING** | No bounded ring buffer in userspace; submits decoded packets directly to kernel mixer. No backpressure. | **IMPLEMENT**: Bounded audio FIFO with capacity tracking, underrun/overrun telemetry. |
| **PCM Buffer Ownership** | `userspace/libbos_media/src/bos_media_pipeline.cpp` | **PARTIAL** | Ad-hoc single static buffer `audio_pcm_buf`. No clear state tracking between decoder, queue, and playback. | **STANDARDIZE**: Explicit buffer ownership states (`DECODER_OWNED` $\to$ `QUEUE_OWNED` $\to$ `RELEASED`). |
| **MP4 Demuxer** | `userspace/libbos_media/demux/mp4_demuxer.c` | **PRODUCTION** | Real dynamic ISO BMFF parser (`ftyp`, `moov`, `trak`, `avcC` at `cur+86`, `stsz`, `stco`). | **PRESERVE**: Retain intact; add sample-accurate seeking via `stts` / `stss` / `stco`. |
| **MP3 Decoder** | `userspace/libbos_media/audio/mp3_decoder.c` | **PRODUCTION** | Real freestanding `minimp3` wrapper producing signed 16-bit stereo PCM. | **PRESERVE**: Retain intact. |
| **Hantro G1 H.264 Engine** | `build/libu_h264.a` | **PRODUCTION** | 29 objects implementing H.264 decoding with FFmpeg CABAC tables. | **PRESERVE**: Retain intact. |
| **AVIO Custom Bridge** | `userspace/libbos_media/src/bos_media_avio.c` | **MISSING** | Demuxing currently calls `BOSMediaStream` directly. No standard AVIO callback interface. | **CREATE**: AVIO-compatible custom I/O wrapper bridging `read_packet` and `seek` to `BOSMediaStream`. |
| **Legacy MPV Adaptor** | `userspace/libbos_media/mpv/` | **LEGACY / UNUSED** | Obsolete Phase 1 mocks (`mpv_instance.cpp`, `mpv_events.cpp`, etc.). | **ISOLATE / EXCLUDE**: Keep unlinked. |
| **Syscall Dispatcher** | `kernel/core/syscall/src/dispatcher.c` | **PRODUCTION** | Dispatches `SYS_OPEN` (14), `SYS_READ` (15), `SYS_SEEK` (26), `SYS_CLOSE` (25), `SYS_AUDIO_CALL` (43). | **PRESERVE**: Retain existing dispatch table. |
| **Syscall Services & Validation** | `kernel/core/syscall/src/services.c`, `validation.c` | **PRODUCTION / PARTIAL** | Full user range validation `[0x40000000, 0x80000000)`. However, `sys_service_audio_call` lacks pointer validation on `pkt` and `pkt->pcm_data`. | **PATCH**: Add user pointer validation in `sys_service_audio_call(STREAM_WRITE)` to guarantee kernel crash immunity. |
| **Kernel VFS Core** | `kernel/vfs/vfs_legacy/src/vfs.c` | **PRODUCTION / PARTIAL** | Full FD table, mount manager, pread/read/seek. `vfs_seek` treats offset as unsigned `uint64_t`, risking overflow on negative offsets for `SEEK_CUR` / `SEEK_END`. | **PATCH**: Cast offset to signed `int64_t` in `vfs_seek()` and validate bounds against file size. |
| **Kernel BOFS Engine** | `kernel/vfs/bofs/src/bofs_vfs.c` | **PRODUCTION** | Native transactional filesystem with WAL, security credentials, and directory tree. | **PRESERVE**: Retain intact. |
| **Audio HAL & Intel HDA** | `kernel/audio/` | **PRODUCTION** | Hardware-abstracted audio core with DMA ring buffers, mixer, and HDA/AC97 drivers. | **PRESERVE**: Retain intact; add cleanup on process termination. |
| **Process Teardown & Reap** | `kernel/core/process/process_manager.c` | **PRODUCTION / PARTIAL** | Terminates tasks and closes surfaces. Does not clean up orphaned audio streams or open FDs. | **PATCH**: Add audio stream destruction and FD cleanup for terminating PID. |

---

## 3. Detailed Root Cause Analysis of Specific Defects

### 3.1 Unbuffered VFS Media Streaming
- **Observed Behavior:** Every audio packet (typically 1152 samples = 2304 bytes) or video slice causes a synchronous context switch into Ring-0 via `SYS_READ`. For a 49 MB media file, this generates tens of thousands of individual syscalls.
- **Root Cause:** `bos_media_stream.c` forwards `read()` directly to `__atoms_syscall3(SYS_READ, fd, buf, count)` without an intermediate ring or block buffer.
- **Architectural Remedy:** Implement a 32 KB or 64 KB read-ahead buffer in userspace `BOSMediaStream`. Small reads (demuxer headers, box lengths) are served directly from the buffer with zero syscalls (`[MEDIA-P3] VFS_CACHE_HIT`). Refills occur in bulk chunks (`[MEDIA-P3] VFS_REFILL`).

### 3.2 VFS Seek Signed Arithmetic & Bounds Violation
- **Observed Behavior:** `vfs_seek(fd, offset, whence)` defines `uint64_t offset`. When seeking backwards (e.g. `SEEK_CUR, -1024` or `SEEK_END, -500`), the negative integer becomes a massive unsigned value (`0xFFFFFFFFFFFFFC00`), setting `g_fd_table[fd].offset` far past EOF.
- **Root Cause:** Lack of signed integer handling in `vfs_seek()` in `vfs.c`.
- **Architectural Remedy:** Treat `offset` as `int64_t` in `vfs_seek()`, clamp negative result to 0, and verify it does not exceed file bounds.

### 3.3 Missing Audio Format Negotiation & Resampling
- **Observed Behavior:** `bos_media_pipeline.cpp` sets `sample_rate = 44100` regardless of the stream's true sample rate. If a video file has 48,000 Hz or 24,000 Hz audio, playback runs at the wrong pitch and speed.
- **Root Cause:** No format negotiation layer between the audio decoder and Audio HAL.
- **Architectural Remedy:** Introduce `BOSAudioStream` format negotiation. Check stream sample rate against Audio HAL capabilities; if the backend accepts 48,000 Hz or 44,100 Hz directly, configure the stream accordingly.

### 3.4 Missing Kernel Pointer Validation in `SYS_AUDIO_CALL`
- **Observed Behavior:** `sys_service_audio_call(ATOMS_AUDIO_OP_STREAM_WRITE, stream_id, (uint64_t)pkt)` casts `(const AudioPcmPacket*)a2` directly and dereferences `pkt->pcm_data` without calling `syscall_validate_user_ptr()`.
- **Root Cause:** Trusting a userspace pointer across the syscall boundary.
- **Architectural Remedy:** Wrap `pkt` with `syscall_validate_user_ptr(pkt, sizeof(AudioPcmPacket))` and `pkt->pcm_data` with `syscall_validate_user_ptr(pkt->pcm_data, pkt->size_bytes)`. Return clean error if invalid.

### 3.5 Timeline Seeking Stub
- **Observed Behavior:** Pressing left/right arrow keys in `media_player` triggers `bos_media_seek(m_player, target)` which returns `BOS_MEDIA_OK` without seeking.
- **Root Cause:** `bos_media_pipeline_seek()` is an empty stub.
- **Architectural Remedy:** Map target millisecond timestamp to target video sample index using demuxer time-to-sample tables (`stts`), seek `BOSMediaStream` to the corresponding chunk offset, reset/flush the Hantro G1 decoder storage (`h264bsdInit`), and resume playback.

---

## 4. Forensic Verdict & Proceed Criteria

The forensic audit confirms that:
1. The Phase 2 decoding core (Hantro G1 + FFmpeg CABAC + minimp3) is solid, freestanding, and working.
2. The necessary kernel primitives (`SYS_OPEN`, `SYS_READ`, `SYS_SEEK`, `SYS_CLOSE`, `SYS_AUDIO_CALL`, `SYS_GUI_MAP_SURFACE`) exist and work.
3. The defects identified are localized strictly to the **bridge layers**, **buffering semantics**, **format negotiation**, and **safety validations**.

**Next Step:** Author `docs/media/PHASE3_ARCHITECTURE.md` and `docs/media/PHASE3_PATCH_PLAN.md` prior to modifying any code.
