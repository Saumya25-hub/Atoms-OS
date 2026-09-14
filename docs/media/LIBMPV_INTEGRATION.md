# ATOMS OS — Native Media Engine Integration Guide
**Document ID:** `docs/media/LIBMPV_INTEGRATION.md`  
**Subsystem:** ATOMS Native Media Framework (`libbos_media`)  
**Date:** September 12, 2026  
**Status:** COMPLETE INTEGRATION GUIDE  

---

## 1. Overview & Architecture

The ATOMS Native Media Engine provides an enterprise-grade, public-freedom media playback framework for ATOMS OS applications. Built on top of an LGPLv2.1+ configured `libmpv` core, it exposes an ergonomic, stable C/C++ API (`libbos_media`) while completely isolating applications from upstream decoder and demuxer internals.

```
+--------------------------------------------------------------+
|             ATOMS Media Player / Explorer / Apps             |
+--------------------------------------------------------------+
                                |
                                v
+--------------------------------------------------------------+
|                     libbos_media C API                       |
|   (bos_media_create, bos_media_open, bos_media_play, etc.)   |
+--------------------------------------------------------------+
        |                                       |
        v                                       v
+-------------------------------+   +--------------------------+
|       mpv_stream_cb           |   |    mpv_render_context    |
| (ATOMS VFS / USB / BOFS)      |   | (BOSurface / ARGB Blit)  |
+-------------------------------+   +--------------------------+
                                |
                                v
+--------------------------------------------------------------+
|                     SYS_AUDIO_CALL Bridge                    |
|             (48kHz S16_LE Stereo -> Intel HDA DMA)           |
+--------------------------------------------------------------+
```

---

## 2. API Quick Reference (`bos_media.h`)

### 2.1 Lifecycle Management
```c
#include <bos_media.h>

// 1. Create player instance
BOSMediaPlayer* player = bos_media_create();

// 2. Attach output video canvas & surface
bos_media_set_video_surface(player, &my_canvas_surface);

// 3. Open media URI (supports USB, PXE RAMDISK, or relative paths)
// Examples:
//   "atoms://storage/USB0/TEST.MP4"
//   "/volumes/usb0/TEST.MP4"
//   "/DOLBY.MP4"
//   "/HEROES.MP3"
int err = bos_media_open(player, "/volumes/usb0/TEST.MP4");
if (err != BOS_MEDIA_OK) {
    // Handle error
}

// 4. Playback control
bos_media_play(player);
bos_media_pause(player);
bos_media_seek(player, 30000); // Seek to 30.0s (30,000 ms)
bos_media_set_volume(player, 0.85f); // 85% volume

// 5. Query state and metadata
BOSMediaState state = bos_media_get_state(player);
BOSMediaMetadata meta;
bos_media_get_metadata(player, &meta);

// 6. Pump events / tick in GUI loop
bos_media_tick(player);

// 7. Cleanup
bos_media_destroy(player);
```

---

## 3. Telemetry & Diagnostics

The engine outputs structured telemetry to serial and UDP port 9999 for automated diagnostics:

### 3.1 Stage Telemetry (`[MEDIA]`)
```text
[MEDIA] OPEN: PASS uri=/volumes/usb0/TEST.MP4
[MEDIA] DEMUX: PASS format=mp4 duration=93.4s
[MEDIA] TRACK_SELECT: video=h264 audio=mp3
[MEDIA] DECODER_INIT: PASS codec=h264 profile=High hwdec=SOFTWARE_FALLBACK
[MEDIA] PACKET_READ: PASS
[MEDIA] FRAME_DECODE: PASS w=1920 h=1080
[MEDIA] SURFACE_BIND: PASS target=0x1004000
[MEDIA] PRESENT: PASS fps=24.0
```

### 3.2 Synchronization Telemetry (`[MEDIA_SYNC]`)
```text
[MEDIA_SYNC] AUDIO_PTS=1420 VIDEO_PTS=1420 DRIFT_MS=0 DROPPED_FRAMES=0 BUFFER_MS=350
```

---

## 4. Building the Media Engine

The media subsystem is integrated into the primary ATOMS build system (`build.ps1`):

```powershell
# Compile libbos_media static library
clang++ -std=c++20 -target x86_64-unknown-none-elf -D_GNU_SOURCE -nostdinc -nostdinc++ ... `
    -Iuserspace/libbos_media/include ... `
    -c userspace/libbos_media/*.cpp

llvm-ar rcs build/libbos_media.a build/bos_media*.o build/mpv_*.o

# Link into Media Player ELF
ld.lld -T userspace/linker.ld atoms/userspace/runtime/crt0.o `
    build/media_player.o build/libbos_ui_cpp.a build/libbos_media.a `
    atoms/userspace/runtime/libatoms_cpp.a atoms/userspace/runtime/libatoms_c.a `
    -o build/media_player.elf
```
