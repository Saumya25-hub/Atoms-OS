# ATOMS OS — First Native C++ Media Player: Architecture Patch Plan
**Document ID**: `NATIVE_MEDIA_PLAYER_PATCH_PLAN.md`  
**Protocol Phase**: TASK 2 — ARCHITECT TEAM  
**Input**: [`docs/media/NATIVE_MEDIA_PLAYER_FORENSIC_REPORT.md`](file:///d:/Signatures_OS/docs/media/NATIVE_MEDIA_PLAYER_FORENSIC_REPORT.md)  
**Date**: September 10, 2026  
**Status**: PROPOSED — PENDING APPROVAL (NO CODE MODIFIED)  

---

## 1. Overview & Architectural Scope

This plan specifies the precise changes required to implement, build, package, and validate the **First Native BOS C++ Media Player** for ATOMS OS.

The architecture strictly adheres to all user mandates:
- **No Mock / Fake Media**: The player exclusively consumes genuine media assets:
  - `TEST1[TEMP]/Dolby_Vision_AtmosHDR.mp4`
  - `TEST1[TEMP]/NCSJanjiHeroesTonight.mp3`
- **Current BOS GUI Architecture**: Built on `BOSurface v2.5` (`bos::Application`, `bos::Window`, `bos::Widget`, `bos::Surface`).
- **Existing Media Pipelines**: Invokes the BOSpectra video pipeline (`mp4_demux`, `h264bsd`, `master_clock`, `render_manager`) and the BOS audio engine (`minimp3`, `audio_mixer`, `intel_hda`).
- **Minimal UI**: Opening the application launches the window, grants focus to the video surface occupying the primary content area, and automatically starts video playback and concurrent audio playback.
- **Universal GPU Acceleration**: Queries the BOS Video Acceleration HAL without vendor hardcoding (Intel / NVIDIA / AMD / Software fallback).
- **No Faking HDR / Dolby Vision**: Inspects and reports the real stream format (H.264 High Profile Level 4.0, 1920x1080, 24 FPS, BT.709 SDR).
- **Crash Isolation**: All media errors, packet boundaries, and memory allocations are isolated from the OS desktop shell, compositor, and kernel.

---

## 2. File Modification & Creation Inventory

| File Path | Action | Subsystem | Purpose & Non-Obvious Rationale |
|-----------|--------|-----------|----------------------------------|
| `tools/gpt_image_builder.c` | **MODIFY** | Disk Builder | Embed `TEST1[TEMP]/Dolby_Vision_AtmosHDR.mp4` as `/DOLBY.MP4` and `/TEST.MP4`, and `TEST1[TEMP]/NCSJanjiHeroesTonight.mp3` as `/HEROES.MP3` into the FAT32 ESP partition. |
| `kernel/media/bospectra/decoder/common/video_accel.c` | **MODIFY** | Video Accel HAL | Replace RTX 4060 hardcoded strings with generic PCI Display Controller enumeration (`0x03`) detecting Intel (`0x8086`), NVIDIA (`0x10DE`), AMD (`0x1002`), or VM/QEMU, falling back safely to pure software decoding. |
| `kernel/media/bospectra/decoder/include/video_accel.h` | **MODIFY** | Video Accel HAL | Add `BOSPECTRA_ACCEL_BACKEND_AMD_VCN` enum value and vendor ID fields. |
| `userspace/apps/media_player/main.cpp` | **CREATE** | Application | Native C++ Media Player implementing `bos::Application`, `bos::Window`, and `MediaPlayerView : public bos::Widget`. Connects to BOSpectra session and BOS audio, auto-plays on startup, and renders video frames to the surface. |
| `userspace/apps/desktop_shell/main.c` | **MODIFY** | Desktop Shell | Add `APP_MEDIA_PLAYER` entry, desktop icon, and launch handler so the Media Player can be activated directly from the desktop shell interface. |
| `build.ps1` | **MODIFY** | Build Engine | Add compilation of `userspace/apps/media_player/main.cpp` linked with `libbos_ui_cpp.a`, `libatoms_cpp.a`, `libatoms_c.a` into `build/media_player.elf`. |

---

## 3. Subsystem Architectural Details

### 3.1 Test Asset Packaging (`tools/gpt_image_builder.c`)
- **What**: Allocate cluster chains and write directory entries for:
  - `DOLBY.MP4` (38,125,940 bytes)
  - `HEROES.MP3` (3,329,709 bytes)
- **Why**: ATOMS OS runs on bare-metal and pure UEFI QEMU where files must reside on the boot FAT32 volume to be accessible via VFS.
- **Expected Result**: Disk image builder creates valid FAT32 cluster chains for both real media assets without overflowing the 512 MB volume.

### 3.2 Universal Video Acceleration HAL (`video_accel.c` & `video_accel.h`)
- **What**: Update `bospectra_video_accel_init()` to iterate PCI devices using `pci_get_device_count()` and `pci_get_device()`.
  - When Base Class == `0x03` (Display):
    - `vendor_id == 0x8086`: Intel HD/UHD Graphics $\to$ Backend candidate: Intel QuickSync / VAAPI.
    - `vendor_id == 0x10DE`: NVIDIA GPU $\to$ Backend candidate: NVIDIA NVDEC.
    - `vendor_id == 0x1002`: AMD Radeon $\to$ Backend candidate: AMD VCN.
    - Other: Generic / Virtual GPU.
  - Transparently sets active backend to `BOSPECTRA_ACCEL_BACKEND_SOFTWARE` because hardware command buffers/firmware are not initialized, while reporting the detected hardware honestly.
- **Why**: Complies strictly with Section 7 ("Do NOT hardcode RTX 4060... player must ask BOS: What video decoder backend is available?").
- **Expected Result**: Clean vendor detection and honest software fallback with zero hardcoded assumptions.

### 3.3 Native C++ Media Player (`userspace/apps/media_player/main.cpp`)
- **What**: Construct the native C++ Media Player using `BOSurface v2.5`:
  - `class VideoSurfaceWidget : public bos::Widget`:
    - Maintains the video canvas.
    - Receives decoded frames from BOSpectra playback session.
    - Blits RGB32 pixels to `bos::Surface` and triggers `invalidate()`.
  - `main(int argc, char** argv)`:
    - Initialize `bos::Application app(argc, argv)`.
    - Create `bos::Window window("BOS Media Player", 60, 50, 960, 580)`.
    - Instantiate `VideoSurfaceWidget` as root widget.
    - Print forensic startup telemetry header to COM1/console:
      - Video file path, container, detected codec (`H264`), profile (`High`), level (`4.0`), resolution (`1920x1080`), fps (`24`), duration (`93.42s`).
      - Audio file path, codec (`MP3`), sample rate (`48000 Hz`), channels (`2 Stereo`).
      - GPU vendor, device ID, video backend (`Software C99 Rasterizer`), decode mode (`SOFTWARE`).
    - Open MP4 video stream via `playback_session_create("/DOLBY.MP4", &sess_id)`.
    - Open MP3 audio stream via `audio_player_open("/HEROES.MP3")`.
    - Start playback immediately (`PTS = 0`).
    - Enter `app.run()` event loop:
      - On tick: `playback_session_tick(sess_id)` and `audio_player_update()`.
      - Present frames with A/V drift tracking.
- **Why**: Satisfies Sections 2, 3, 4, 5, 6, 9, 10, 11, 13, 14, 15, and 16.
- **Expected Result**: Video displayed smoothly on screen, audio audible via Intel HDA, responsive window interactions.

### 3.4 Desktop Shell Integration (`userspace/apps/desktop_shell/main.c`)
- **What**:
  - Add `APP_MEDIA_PLAYER` to `AppType` enum.
  - Add `{ "Media Player", 120, 24, 76, 76, g_desktop_ico_computer_48, false }` (or second column icon) to `g_icons`.
  - In `open_app()`, configure window title `"BOS Media Player"`, bounds $960 \times 580$, and trigger real playback.
  - In render loop, render the real-time video frame within the player window body.
- **Why**: Allows interactive validation in both standalone ELF execution and integrated desktop shell testing.

### 3.5 Build Engine (`build.ps1`)
- **What**: Compile `userspace/apps/media_player/main.cpp` using Clang++ C++20 freestanding flags and link with `libbos_ui_cpp.a`, `libatoms_cpp.a`, `libatoms_c.a` into `build/media_player.elf`.
- **Expected Result**: Clean compilation and link with zero warnings and zero errors.

---

## 4. Risk Assessment & Rollback Plan

### Risks & Mitigations
1. **Risk**: FAT32 cluster chain corruption when adding 41 MB of files.
   - **Mitigation**: `gpt_image_builder.c` dynamically computes cluster chains starting at Cluster 3 and writes both FAT1 and FAT2 tables before flushing file data.
2. **Risk**: Audio/Video clock drift causing frame starvation.
   - **Mitigation**: `playback_session.c` uses monotonic wall-clock time (`master_clock_get_time_us`), dropping late frames if video falls behind audio $> 15\,\text{ms}$, and holding frames if video is early.
3. **Risk**: High Profile H.264 macroblock decode crash on corrupted packet.
   - **Mitigation**: `h264bsdDecode` return values (`H264BSD_ERROR`, `H264BSD_PARAM_SET_ERROR`) are caught cleanly; corrupted packets are discarded without halting the session.

### Rollback Plan
If any step fails validation:
```bash
git checkout HEAD -- tools/gpt_image_builder.c kernel/media/ userspace/apps/desktop_shell/ build.ps1
rm -rf userspace/apps/media_player/
```
System state is completely restored to Phase M5 certified baseline.
