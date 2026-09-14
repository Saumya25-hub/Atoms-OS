# ATOMS OS — Phase M2: Audio Engine & Codecs Forensic Report
**Protocol Stage**: Task 1 (Forensic Team)  
**Date**: September 10, 2026  
**Auditor**: ATOMS OS Audio & Multimedia Core Engineering  
**Status**: INVESTIGATION COMPLETE — NO CODE MODIFIED (STRICT RULE 0 ISOLATION)

---

## 1. Executive Summary

A comprehensive forensic audit of the ATOMS OS audio subsystem was executed to examine the entire vertical audio datapath:
$$\text{File (BOFS/VFS)} \longrightarrow \text{Demux} \longrightarrow \text{Codec Decoder} \longrightarrow \text{PCM Buffer} \longrightarrow \text{Format Negotiation/Resampler} \longrightarrow \text{Audio Mixer} \longrightarrow \text{Audio HAL} \longrightarrow \text{Hardware Backend (HDA/AC97)} \longrightarrow \text{Physical Output}$$

The audit uncovered critical architectural gaps, hardcoded assumptions, stubs, and missing components that prevent modern real-world audio files (MP3, FLAC, AAC, Opus, Vorbis, and variable-rate WAV) from playing:

1. **Zero Compressed Audio Codecs**:
   There are **no** compressed audio decoders implemented anywhere in the kernel or userspace. Zero support exists for MP3, FLAC, AAC, Opus, or Ogg Vorbis.
2. **Hardcoded WAV Format Enforcement**:
   The existing WAV session in `kernel/audio/session/audio_player.c` strictly rejects all files unless they are exactly **48,000 Hz, 16-bit, 2-channel Stereo PCM**. Common 44.1 kHz, 8/24/32-bit, or mono WAV files are immediately rejected.
3. **No Audio Resampler or Channel Engine**:
   The software mixer (`kernel/audio/mixer/audio_mixer.c`) silently drops any audio stream whose sample rate or channel count does not identically match the hardware output target (`target_format`). Streams of 44.1 kHz are completely ignored.
4. **Intel HDA DMA Output Loop Disconnected**:
   While the Intel HDA controller and Realtek codec initialization were implemented in Phase M1, the HDA playback stream in `kernel/audio/drivers/hda/bos_hda_adapter.c` only pre-fills the DMA ring with a static 440 Hz diagnostic tone. Its update callback (`bos_hda_stream_update_pointers`) **never** pulls PCM data from `audio_mixer_process()`. Consequently, HDA loops the test tone and cannot stream real audio from files.
5. **Userspace Audio Bridge Stubs & Missing Syscalls**:
   `userspace/libs/audio/audio_user.c` contains multiple no-ops (`audio_stream_set_format`, `audio_stream_flush`, `audio_stream_reset`). Furthermore, kernel syscall gateway (`kernel/core/syscall/src/services.c`) lacks handlers for `STREAM_SET_FORMAT`, `STREAM_FLUSH`, `STREAM_RESET`, and `DEVICE_GET_CAPS`, and dereferences userspace packet pointers without memory safety validation.
6. **BOSpectra Multimedia Audio Bridge Discards PCM**:
   In `kernel/media/bospectra/audio/bridge/audio_bridge.c`, `bospectra_audio_bridge_write_pcm` ignores the incoming PCM buffer and merely calls `ac97_playback_update()`, discarding all decoded audio samples.
7. **Dead/Duplicate Subsystem (`kernel/audio_v3`)**:
   `kernel/audio_v3` is an orphaned clone of `kernel/audio` that is not compiled by `build.ps1` or referenced in `build/link.rsp`.

---

## 2. Detailed Forensic Findings by Subsystem

### 2.1 Codec Layer & Audio Decoders
- **Location**: `kernel/audio/session/audio_player.c`, `kernel/media/bospectra/decoder/`
- **Findings**:
  - **Compressed Codecs Missing**: No MP3, FLAC, AAC, Opus, or Vorbis decoders exist in the repository.
  - **WAV Hardcoding**: Lines 184–188 of `audio_player.c`:
    ```c
    if (fmt.audio_format != 1 || fmt.num_channels != 2 || fmt.sample_rate != 48000 || fmt.bits_per_sample != 16) {
        display_print("[AUDIO_PLAYER] Unsupported format (must be 48kHz 16-bit stereo PCM)!\n");
        vfs_close(fd);
        return;
    }
    ```
    Any WAV file with sample rate 44.1 kHz, 22.05 kHz, 88.2 kHz, 96 kHz, mono, or 24-bit/32-bit depth is aborted with an error.
  - **No Tag / Metadata Parsing**: ID3v1, ID3v2, Vorbis Comments, and MP4 metadata atoms are completely absent.

### 2.2 Software Mixer & Resampling Engine
- **Location**: `kernel/audio/mixer/audio_mixer.c`
- **Findings**:
  - **Silent Stream Discard**: Lines 119–122:
    ```c
    if (stream->format.format != target_format->format ||
        stream->format.channels != target_format->channels) {
        continue;
    }
    ```
    Streams with different formats or channel counts (such as mono speech) are dropped without conversion.
  - **Zero Resampling Capability**: There is no resampler module in `kernel/audio`. If a stream is 44.1 kHz and the hardware is running at 48.0 kHz, the mixer has no rate converter, causing severe pitch drift or silence.
  - **Hardcoded Default Stream Rate**: In `kernel/audio/streams/audio_stream.c` (line 30), streams default to 44.1 kHz (`stream->format.sample_rate = 44100;`), while `audio_mixer_process` expects 48.0 kHz.

### 2.3 Intel High Definition Audio (HDA) Output Datapath
- **Location**: `kernel/audio/drivers/hda/bos_hda_adapter.c`
- **Findings**:
  - **Diagnostic Tone Pre-Fill Only**: Lines 97–98:
    ```c
    /* Pre-fill buffer with 440 Hz reference diagnostic tone */
    hda_generate_test_tone((int16_t*)ctrl->dma_buffer_virt, ctrl->dma_buffer_size / sizeof(int16_t));
    ```
  - **No Realtime Refill Loop**: In `bos_hda_stream_update_pointers()` (lines 226–232):
    ```c
    void bos_hda_stream_update_pointers(uint32_t frames_written) {
        bos_hda_controller_t* ctrl = bos_hda_get_controller();
        if (!ctrl) return;

        uint32_t bytes = frames_written * 4;
        ctrl->write_pos = (ctrl->write_pos + bytes) % ctrl->dma_buffer_size;
    }
    ```
    Unlike `ac97_playback.c` which tracks the DMA position (`CIV`) and actively calls `audio_mixer_process()` to write new audio into the ring buffer, HDA does **not** call `audio_mixer_process()`. The DMA hardware continues looping the initial diagnostic tone or stale memory.
  - **Hardware Format Locked**: Format register `HDA_SD_FMT` is hardcoded to `0x0011` (48 kHz, 16-bit, 2ch). Dynamic capability negotiation with the codec DAC is not wired to the mixer.

### 2.4 Legacy AC97 Driver
- **Location**: `kernel/audio/drivers/ac97/ac97_playback.c`
- **Findings**:
  - **Functional Refill**: AC97 correctly calls `audio_mixer_process(target, chunk_bytes, &g_ac97_format)` on lines 467–470 and advances its BDL ring descriptors.
  - **Format Fixed at 48kHz**: Fixed to 48kHz stereo 16-bit. Resampling is required to play 44.1kHz audio through it.

### 2.5 Userspace Audio API & Syscall Gateway
- **Location**: `userspace/libs/audio/audio_user.c`, `kernel/core/syscall/src/services.c`
- **Findings**:
  - **No-Op Functions**:
    - `audio_stream_set_format`: Returns `true` immediately without executing a syscall.
    - `audio_stream_available`: Returns hardcoded `4096`.
    - `audio_stream_capacity`: Returns hardcoded `65536`.
    - `audio_stream_flush` / `audio_stream_reset`: Empty return `true`.
  - **Missing Syscalls**:
    - `userspace/runtime/c/include/atoms_syscall.h` and `kernel/core/syscall/include/syscall.h` only define 9 opcodes (`ATOMS_AUDIO_OP_DEVICE_GET_INFO` to `ATOMS_AUDIO_OP_DEVICE_SET_VOL`).
    - Opcode for format setting (`ATOMS_AUDIO_OP_STREAM_SET_FORMAT`) does not exist.
  - **Memory Safety Risk**:
    In `sys_service_audio_call` (line 1263):
    ```c
    case ATOMS_AUDIO_OP_STREAM_WRITE: {
        if (!a2) return 0;
        return (uint64_t)audio_stream_write((uint32_t)a1, (const AudioPcmPacket*)a2);
    }
    ```
    The kernel blindly casts userspace address `a2` to `const AudioPcmPacket*` and dereferences `packet->pcm_data` without verifying pointer bounds or copying the descriptor into kernel space.

### 2.6 Container Demuxer & Multimedia Integration
- **Location**: `kernel/media/bospectra/audio/bridge/audio_bridge.c`, `kernel/media/bospectra/container/mp4/mp4_parser.c`
- **Findings**:
  - **Discarded PCM**: In `audio_bridge.c` (lines 32–41):
    ```c
    bospectra_error_t bospectra_audio_bridge_write_pcm(const uint8_t* pcm_data, size_t size_bytes, const BOSPECTRA_AudioSpec* spec) {
        (void)spec;
        if (!g_audio_bridge_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
        if (!pcm_data || size_bytes == 0) return BOSPECTRA_ERR_INVALID_ARGUMENT;

        // Transmit PCM frame buffer into existing AC97 playback worker loop
        ac97_playback_update();

        return BOSPECTRA_SUCCESS;
    }
    ```
    Incoming PCM frames from container playback are completely discarded (`(void)pcm_data`).
  - **MP4 AAC Audio Track**: `TEST.MP4` has Track 1 containing AAC audio (`mp4a`), but the demuxer reports `[AUDIO] VIDEO PASS / AUDIO NOT YET SUPPORTED` because no AAC decoder exists.

### 2.7 Code Redundancy & Dead Subsystems
- **Location**: `kernel/audio_v3/`
- **Findings**:
  - `kernel/audio_v3` is an unlinked, older duplicate of `kernel/audio`.
  - It contains 26 source/header files that are not compiled in `build.ps1`.
  - Maintaining both directories causes confusion and namespace drift.

---

## 3. Root Cause Classification Matrix

| Component | Nature of Defect | Severity | Impact |
|---|---|---|---|
| **Audio Codecs** | Completely missing (MP3, FLAC, AAC, Opus, Vorbis) | **Critical** | Cannot play any compressed audio files |
| **WAV Parser** | Hardcoded restriction (48kHz/16-bit/stereo only) | **High** | Cannot play standard 44.1kHz or mono/24-bit WAVs |
| **Resampler** | Missing rate conversion engine | **Critical** | Cannot convert 44.1kHz audio to 48kHz hardware rate |
| **Channel Engine** | Mono/Multi-channel conversion missing | **High** | Cannot downmix or upmix mono streams |
| **HDA Playback** | DMA refill loop disconnected from mixer | **Critical** | Loops static 440Hz test tone; no real file streaming |
| **Syscall Bridge** | Missing format opcode; unsafe userspace pointer dereference | **High** | Userspace applications cannot set format or stream safely |
| **BOSpectra Bridge** | `bospectra_audio_bridge_write_pcm` drops PCM bytes | **High** | Video container audio tracks cannot be heard |
| **`kernel/audio_v3`**| Orphaned/duplicate tree | **Low** | Maintenance deadweight |

---

## 4. Suspected Architecture Fixes (Forensic Recommendations - NO CODE)

1. **Third-Party Codec Isolation (`third_party/audio/`)**:
   - Integrate mature, portable, freestanding, permissively licensed (MIT / CC0 / BSD / Apache-2.0) C99 decoders:
     - **WAV/PCM**: Universal WAV parser supporting 8/16/24/32-bit, extensible WAV, mono/stereo (e.g. `dr_wav`).
     - **MP3**: Freestanding MP3 decoder with ID3v1/ID3v2 metadata parsing (e.g. `minimp3`).
     - **FLAC**: Freestanding FLAC decoder supporting 16/24-bit, seek tables, Vorbis comments (e.g. `dr_flac`).
     - **Vorbis**: Portable Vorbis decoder (e.g. `stb_vorbis`).
     - **AAC**: Integer-only AAC-LC decoder for standalone ADTS and MP4/M4A audio tracks.
     - **Opus**: Freestanding fixed-point Opus decoder for Ogg Opus.
   - All third-party decoders must reside strictly in `third_party/audio/<codec>/` with full license documentation.
2. **Canonical BOS Audio Format & Resampler**:
   - Create a clean `AudioPcmFormat` descriptor supporting arbitrary sample rates, bit depths, and channel configurations.
   - Implement a fast integer/fixed-point linear/polyphase resampler converting arbitrary input rates (44.1k, 22k, 88.2k, 96k) to the hardware target rate (48k).
   - Implement a channel downmix/upmix matrix (mono $\to$ stereo, stereo $\to$ mono).
3. **Universal Audio Mixer Upgrade**:
   - Enhance `audio_mixer.c` with automatic resampling and channel conversion for each active stream.
   - Add master/stream volume scaling, soft saturation/clipping protection, and underrun tracking.
4. **HDA Realtime Playback Refill**:
   - Implement an active DMA ring refill worker in `bos_hda_adapter.c` that queries hardware `LPIB`, detects consumed blocks, and refills the DMA buffer with mixed PCM data from `audio_mixer_process()`.
5. **Userspace Audio Bridge & Syscall Hardening**:
   - Add `ATOMS_AUDIO_OP_STREAM_SET_FORMAT`, `ATOMS_AUDIO_OP_STREAM_FLUSH`, `ATOMS_AUDIO_OP_STREAM_RESET`, and `ATOMS_AUDIO_OP_DEVICE_GET_CAPS` to syscall gateway.
   - Safely validate userspace memory buffers before writing into the stream ring buffer.
6. **Container Audio Integration**:
   - Connect the MP4 demuxer's audio stream (`mp4a`) to the AAC decoder and pipe decoded PCM into the Audio HAL / mixer.
   - Replace the discarded PCM stub in `bospectra_audio_bridge_write_pcm` with real stream writes.
7. **Clean Build System**:
   - Compile all new codecs with `-target x86_64-pc-none-elf -msoft-float -mno-sse -mno-sse2 -ffreestanding -mno-red-zone`.
   - Remove or deprecate `kernel/audio_v3`.

---

## 5. Protocol Stage Conclusion

Task 1 (Forensic Audit) is complete. No source code was modified during this audit.
All findings will be ingested by Task 2 (Architect Team) to generate `docs/media/M2_AUDIO_PATCH_PLAN.md` and `docs/media/M2_THIRD_PARTY_AUDIO_LICENSE_AUDIT.md`.
