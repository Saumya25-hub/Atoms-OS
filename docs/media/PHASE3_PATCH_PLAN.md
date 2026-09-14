# ATOMS OS — Phase 3 Architectural Patch Plan
**Subsystem:** Userspace Media Engine ➔ VFS + Audio Userspace Bridges  
**Milestone:** Phase 3 Patch Plan (Scope Control & Risk Management)  
**Date:** September 12, 2026  
**Status:** **APPROVED FOR IMPLEMENTATION (RULE 0 ENFORCED)**  

---

## 1. Scope Control & Files to Modify

| Target File | Purpose | Proposed Changes | Rationale & Safety |
|:---|:---|:---|:---|
| `userspace/libbos_media/include/bos_media_stream.h`<br>`userspace/libbos_media/src/bos_media_stream.c` | VFS Media Stream Bridge | Add `eof()`, `error()`, stream instance pool, 64 KB read-ahead cache with hit/miss/refill metrics, signed seek calculation, and `[MEDIA-P3]` telemetry. | Reduces context switch latency by serving small demuxer reads from cache; eliminates static single-stream limitation. |
| `userspace/libbos_media/include/bos_media_avio.h`<br>`userspace/libbos_media/src/bos_media_avio.c` | AVIO Callback Bridge | Implement standard `BOSMediaAVIO` context bridging `read_packet` and `seek` callbacks to `BOSMediaStream` with `[MEDIA-P3] AVIO_*` telemetry. | Standardizes stream I/O interface for ISO BMFF / FFmpeg format demuxers. |
| `userspace/libbos_media/audio/bos_audio_stream.h`<br>`userspace/libbos_media/audio/bos_audio_stream.c` | Audio Userspace Bridge | Implement `BOSAudioStream` with format negotiation, bounded 128 KB FIFO queue, backpressure handling, sample format conversion (FLTP/S32 $\to$ S16_LE), and lifecycle control (`create`, `configure`, `start`, `write`, `drain`, `flush`, `stop`, `destroy`). | Encapsulates audio hardware differences, provides backpressure to decoder, and guarantees non-blocking playback. |
| `userspace/libbos_media/src/bos_media_pipeline.h`<br>`userspace/libbos_media/src/bos_media_pipeline.cpp` | Media Engine Controller | Wire `BOSMediaStream` cache and `BOSAudioStream` queue into the playback loop. Implement sample-accurate video timeline seeking and decoder DPB flush. | Connects demuxing and decoding to the production stream and audio bridges. |
| `userspace/apps/media_player/main.cpp` | Media Center UI | Connect seeking keys ($\leftarrow$ / $\rightarrow$) to `bos_media_seek()`; display cache hit/miss statistics and audio queue depth in header/footer. | Provides user control over seeking and exposes runtime telemetry. |
| `kernel/vfs/vfs_legacy/src/vfs.c`<br>`kernel/vfs/vfs_legacy/include/vfs.h` | Kernel VFS Core | Support signed `int64_t offset` in `vfs_seek()` and clamp to valid range `[0, node->size]`. | Fixes negative seek underflow bug in `SEEK_CUR` and `SEEK_END`. |
| `kernel/core/syscall/src/services.c` | Syscall Gateway Safety | Add `syscall_validate_user_ptr` checks on `AudioPcmPacket` and `pkt->pcm_data` in `sys_service_audio_call(STREAM_WRITE)`. | Prevents rogue/corrupted userspace pointer from crashing the kernel. |
| `kernel/audio/core/audio_core.h`<br>`kernel/audio/core/audio_core.c` | Audio Stream Lifecycle | Implement `audio_core_destroy_streams_by_pid(uint32_t pid)` called during process termination. | Prevents audio stream leaks when media player terminates abruptly. |

---

## 2. Expected Results & Acceptance Criteria

1. **VFS Buffering:** Demuxer box and header reads achieve $\ge 80\%$ cache hit rate (`[MEDIA-P3] VFS_CACHE_HIT`). Refills occur in 64 KB batches (`[MEDIA-P3] VFS_REFILL`).
2. **Seek Semantics:** `SEEK_SET`, `SEEK_CUR`, and `SEEK_END` seek accurately without integer underflow or unbounded offsets. Video timeline seeking flushes the Hantro G1 DPB and restarts decoding cleanly at the new timestamp.
3. **AVIO Bridge:** AVIO callbacks correctly invoke `BOSMediaStream` and report `[MEDIA-P3] AVIO_READ`, `AVIO_SEEK`, and `AVIO_EOF`.
4. **Audio Format Negotiation & FIFO:** Decoded audio (44.1 kHz or 48 kHz, S16_LE or FLTP) negotiates format with Audio HAL and buffers through the bounded FIFO with zero underruns during steady-state playback.
5. **Kernel Robustness:** Submitting invalid pointers, closed FDs, or corrupt packets fails gracefully with negative error codes without triggering kernel panics or page faults.
6. **Teardown Safety:** Process exit cleans up all open VFS streams and Audio HAL handles without resource leaks.

---

## 3. Rollback Plan

If any regression occurs during compilation or testing:
1. Revert modifications in `kernel/core/syscall/src/services.c`, `kernel/vfs/vfs_legacy/src/vfs.c`, and `kernel/audio/core/audio_core.c`.
2. Restore `userspace/libbos_media/` and `userspace/apps/media_player/` to the certified Phase 2 baseline.
3. Re-execute `run_phase2_qemu_verification.ps1` to re-verify Phase 2 stability.
