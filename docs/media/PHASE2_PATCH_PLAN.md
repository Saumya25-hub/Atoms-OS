# ATOMS OS — Phase 2 Architectural Patch Plan
**Subsystem:** Userspace Media Engine ➔ Real Mature Media Integration  
**Milestone:** Phase 2C (Implementation Plan & Scope Control)  
**Date:** September 12, 2026  
**Status:** **APPROVED FOR IMPLEMENTATION (RULE 0 ENFORCED)**  

---

## 1. What to Modify & Why

| Target File | Subsystem / Purpose | Proposed Changes | Rationale & Safety |
|:---|:---|:---|:---|
| `userspace/libbos_media/include/bos_media_stream.h` | Media Stream Interface | Declare `BOSMediaStream` interface (`read`, `seek`, `tell`, `close`, `size`) | Standardizes stream I/O for demuxers without host OS dependencies. |
| `userspace/libbos_media/src/bos_media_stream.c` | VFS Stream Adapter | Implement stream functions using ATOMS syscalls (`SYS_OPEN`, `SYS_READ`, `SYS_SEEK`, `SYS_CLOSE`) | Pure userspace VFS access; zero host/Linux file dependencies. |
| `userspace/libbos_media/demux/mp4_demuxer.h` & `.c` | Dynamic ISO/MP4 Demuxer | Implement dynamic box parser (`ftyp`, `moov`, `trak`, `avcC`, `stsz`, `stco`, `co64`) operating on `BOSMediaStream` | Replaces kernel-coupled `mp4_demux.c` and eliminates hardcoded file offsets. |
| `userspace/libbos_media/src/bos_media_pipeline.cpp` | Genuine Media Pipeline Controller | Implement full playback engine: VFS stream $\to$ Demuxer $\to$ `h264bsd` (with FFmpeg CABAC) / `minimp3` $\to$ YUV420P conversion $\to$ BOSurface blit | Replaces fake mock `mpv_render_context_render()`. Emits mandatory `[MEDIA-P2]` telemetry. |
| `userspace/libbos_media/src/bos_media.cpp` | Public Media API | Wire `bos_media_*` functions directly to `BOSMediaPipeline` | Connects userspace applications to the real media pipeline. |
| `userspace/apps/media_player/main.cpp` | Native Media Player Application | Enhance GUI canvas to blit genuine video frames, render playback status, display video resolution/codec info, and handle keyboard controls | Complete production-grade UI driven by actual decoded frames. |

---

## 2. Expected Results

1. **Genuine Demux**: Dynamic ISO parsing of `Dolby_Vision_AtmosHDR.mp4` / `TEST.MP4` without hardcoded offsets.
2. **Genuine Decode**: H.264 High Profile CABAC decoded using genuine FFmpeg CABAC tables and `h264bsd`.
3. **Genuine Audio**: MP3 decoded to PCM via `minimp3` and streamed to Audio HAL via `SYS_AUDIO_CALL`.
4. **Distinct Frame CRCs**: At least 3 different decoded video frames have distinct CRC32 checksums.
5. **Clean Error Handling**: Corrupt or malformed media files fail gracefully in userspace without kernel panics.

---

## 3. Rollback Plan

If any issue arises during compilation or execution:
1. Revert `userspace/apps/media_player/` and `userspace/libbos_media/` to the certified Phase 1 baseline.
2. Re-link `media_player.elf` with Phase 1 objects.
3. Verify QEMU pre-flight to confirm baseline stability.
