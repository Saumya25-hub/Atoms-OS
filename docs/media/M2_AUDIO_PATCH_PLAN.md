# ATOMS OS — Phase M2: Audio Engine & Codecs Patch Plan
**Document ID**: `docs/media/M2_AUDIO_PATCH_PLAN.md`  
**Protocol Stage**: Task 2 (Architect Team)  
**Date**: September 10, 2026  
**Architect**: ATOMS OS Audio & Core Architecture Authority  
**Status**: APPROVED ARCHITECTURE PLAN — AWAITING EXECUTION (STRICT RULE 0 ISOLATION)

---

## 1. Objectives & Architectural Blueprint

The goal of Phase M2 is to upgrade ATOMS OS from a rigid, hardcoded 48kHz WAV-only player into a **production-grade, universal audio engine** capable of playing modern real-world audio files (WAV, MP3, FLAC, AAC, Vorbis, Opus) across all supported hardware transports (Intel HDA, AC97, and future USB Audio) and exposing the audio pipeline cleanly to userspace.

### End-to-End Architectural Dataflow:
```text
┌──────────────────────────────────────────────────────────────────────────────────┐
│                             FILE & STREAM INPUT                                  │
│   WAV (8/16/24/32-bit) │ MP3 (CBR/VBR) │ FLAC (16/24-bit) │ AAC │ Vorbis │ Opus  │
└────────────────────────────────────────┬─────────────────────────────────────────┘
                                         │
                                         ▼
┌──────────────────────────────────────────────────────────────────────────────────┐
│                           BOS CODEC ADAPTER LAYER                                │
│  third_party/audio/wav  (dr_wav)       │ third_party/audio/mp3    (minimp3)      │
│  third_party/audio/flac (dr_flac)      │ third_party/audio/vorbis (stb_vorbis)   │
│  third_party/audio/aac  (helix-aac)    │ third_party/audio/opus   (opus-fixed)   │
└────────────────────────────────────────┬─────────────────────────────────────────┘
                                         │ Decoded Raw PCM (Arbitrary Rate / Ch)
                                         ▼
┌──────────────────────────────────────────────────────────────────────────────────┐
│                      FORMAT NEGOTIATOR & CHANNEL ENGINE                          │
│   - Channel Matrix: Mono -> Stereo duplicate, Multi-channel downmix              │
│   - Bit Depth Conversion: 8-bit / 24-bit / 32-bit -> Signed 16-bit PCM           │
└────────────────────────────────────────┬─────────────────────────────────────────┘
                                         │
                                         ▼
┌──────────────────────────────────────────────────────────────────────────────────┐
│                        INTEGER RESAMPLER ENGINE (16.16)                          │
│   44.1 kHz / 22.05 kHz / 88.2 kHz / 96.0 kHz ───> 48.0 kHz Hardware Target Rate  │
└────────────────────────────────────────┬─────────────────────────────────────────┘
                                         │
                                         ▼
┌──────────────────────────────────────────────────────────────────────────────────┐
│                        UNIVERSAL AUDIO MIXER ENGINE                              │
│   - Up to 32 simultaneous concurrent streams                                     │
│   - Per-stream volume & Master volume control (0..255)                           │
│   - 32-bit accumulator with soft-saturation / anti-clipping protection           │
│   - Underrun / overflow flight recording telemetry                               │
└────────────────────────────────────────┬─────────────────────────────────────────┘
                                         │
                                         ▼
┌──────────────────────────────────────────────────────────────────────────────────┐
│                             BOS AUDIO HAL (Unified)                              │
│                ┌───────────────────────┴───────────────────────┐                  │
│                ▼                                               ▼                  │
│   Intel HDA Driver (MMIO DMA)                    Legacy AC97 Driver (BDL DMA)    │
│   - Hardware LPIB tracking                       - CIV / LVI ring management     │
│   - Realtime circular buffer refill              - Realtime BDL descriptor refill│
│   - Dynamic DAC / Pin negotiation                - 48 kHz stereo 16-bit output   │
└────────────────┬───────────────────────────────────────────────┬─────────────────┘
                 ▼                                               ▼
          Physical HDA Codec                             Physical AC97 Codec
          (Realtek ALC on H81)                           (QEMU / Legacy HW)
                 │                                               │
                 ▼                                               ▼
       [REAL PHYSICAL SOUND]                           [REAL PHYSICAL SOUND]
```

---

## 2. Third-Party Codec Components & Directory Layout

Strictly adhering to **Rules 2, 3, and 4**:
- All third-party codec implementations reside exclusively under `third_party/audio/`.
- Every subfolder contains upstream source, `LICENSE`, `NOTICE`, and `README.BOS`.
- External code is **never** presented as original BOS code.

```text
third_party/audio/
├── LICENSE                                 (Master License Manifest)
├── intel_hda/                              (HDA register constants, BSD-2)
│   ├── LICENSE
│   ├── README.BOS
│   └── include/hda_reg.h, hda_codec.h
├── wav/                                    (dr_wav, MIT-0 / Public Domain)
│   ├── LICENSE
│   ├── README.BOS
│   └── include/dr_wav.h
├── mp3/                                    (minimp3, CC0-1.0)
│   ├── LICENSE
│   ├── README.BOS
│   └── include/minimp3.h, minimp3_ex.h
├── flac/                                   (dr_flac, MIT-0 / Public Domain)
│   ├── LICENSE
│   ├── README.BOS
│   └── include/dr_flac.h
├── vorbis/                                 (stb_vorbis, MIT / Public Domain)
│   ├── LICENSE
│   ├── README.BOS
│   └── include/stb_vorbis.h
└── aac/                                    (Fixed-Point AAC-LC, Apache-2.0)
    ├── LICENSE
    ├── README.BOS
    └── include/aac_decoder.h
```

---

## 3. Subsystem Modifications & File Inventory

### 3.1 BOS Codec Registry & Adapters (`kernel/audio/codecs/`)
- **[NEW] `kernel/audio/codecs/bos_audio_codec.h`**:
  Defines `BOSAudioCodecDriver` and `BOSAudioDecoder` vtable interfaces:
  - `probe(data, size)`: Magic bytes / header inspection.
  - `open(dec, data, size)`: Allocate decoder context and parse metadata (ID3, Vorbis Comments).
  - `decode_frame(dec, pcm_out, max_samples, samples_out, format)`: Decode compressed frames to PCM.
  - `seek(dec, sample_idx)`: Seeking support.
  - `close(dec)`: Teardown.
- **[NEW] `kernel/audio/codecs/wav_codec.c`**:
  Wraps `dr_wav`. Decodes 8/16/24/32-bit PCM and IEEE float WAV files of any sample rate or channel configuration.
- **[NEW] `kernel/audio/codecs/mp3_codec.c`**:
  Wraps `minimp3`. Decodes CBR and VBR MP3 streams, parses ID3v1 and ID3v2 tags (title, artist, album), and produces 16-bit PCM.
- **[NEW] `kernel/audio/codecs/flac_codec.c`**:
  Wraps `dr_flac`. Decodes 16-bit and 24-bit FLAC audio blocks with Vorbis comment metadata parsing.
- **[NEW] `kernel/audio/codecs/vorbis_codec.c`**:
  Wraps `stb_vorbis`. Decodes Ogg Vorbis streams into 16-bit stereo/mono PCM.
- **[NEW] `kernel/audio/codecs/aac_codec.c`**:
  Freestanding fixed-point AAC-LC decoder for raw ADTS streams and container-demuxed MP4 `mp4a` audio frames.
- **[NEW] `kernel/audio/codecs/codec_registry.c` / `.h`**:
  Central driver registry that detects format from magic bytes or extension and creates the appropriate decoder.

---

### 3.2 Channel Engine & Integer Resampler (`kernel/audio/mixer/`)
- **[NEW] `kernel/audio/mixer/audio_resampler.h` / `audio_resampler.c`**:
  - Implements a pure 16.16 fixed-point linear/polyphase resampler.
  - Converts arbitrary input rates ($F_{\text{in}} \in \{8000, 11025, 22050, 44100, 88200, 96000\}$ Hz) to target hardware rate ($F_{\text{out}} = 48000$ Hz) with zero floating-point registers.
  - Memory footprint: Stack-bounded, zero dynamic allocation on the mixing hot path.
- **[NEW] `kernel/audio/mixer/audio_channel.h` / `audio_channel.c`**:
  - Implements mono-to-stereo channel duplication ($L = R = S$) and stereo-to-mono downmixing ($M = (L + R) / 2$).
- **[MODIFY] `kernel/audio/mixer/audio_mixer.c`**:
  - Remove silent stream discard (`continue`).
  - Automatically feed streams through the channel engine and resampler if their format differs from `target_format`.
  - Add soft-saturation anti-clipping math (`audio_math_mix_16` / `audio_volume_apply_16`).

---

### 3.3 Intel HDA Driver Realtime DMA Streaming (`kernel/audio/drivers/hda/`)
- **[MODIFY] `kernel/audio/drivers/hda/bos_hda_adapter.c`**:
  - Replace the static 440 Hz test tone with an active circular DMA ring refill loop.
  - In `bos_hda_stream_update_pointers()`:
    1. Read Link Position in Buffer (`HDA_SD_LPIB`).
    2. Determine bytes consumed by the DAC since last tick.
    3. Call `audio_mixer_process(target, consumed_bytes, &g_hda_format)`.
    4. Advance write cursor and maintain ring safety margins.
  - Ensure HDA continues uninterrupted alongside AC97.

---

### 3.4 Userspace Audio API & Syscall Gateway (`userspace/` & `kernel/core/syscall/`)
- **[MODIFY] `kernel/core/syscall/include/syscall.h` & `userspace/runtime/c/include/atoms_syscall.h`**:
  Add missing audio opcodes:
  - `#define ATOMS_AUDIO_OP_STREAM_SET_FORMAT 10U`
  - `#define ATOMS_AUDIO_OP_STREAM_FLUSH      11U`
  - `#define ATOMS_AUDIO_OP_STREAM_RESET      12U`
  - `#define ATOMS_AUDIO_OP_STREAM_GET_AVAIL  13U`
  - `#define ATOMS_AUDIO_OP_DEVICE_GET_CAPS   14U`
- **[MODIFY] `kernel/core/syscall/src/services.c`**:
  - In `sys_service_audio_call`:
    - Add handlers for opcodes 10U through 14U.
    - Implement safe user-space memory buffer copying: copy `AudioPcmPacket` header and verify payload pointer with `vmm_validate_user_buffer()` before submitting to kernel ring buffers.
- **[MODIFY] `userspace/libs/audio/audio_user.c`**:
  - Wire `audio_stream_set_format`, `audio_stream_flush`, `audio_stream_reset`, `audio_stream_available`, and `audio_stream_capacity` directly to the new syscall opcodes.

---

### 3.5 Universal Audio Player & File Playback (`kernel/audio/session/`)
- **[MODIFY] `kernel/audio/session/audio_player.c`**:
  - Remove hardcoded RIFF/WAV-only checks and 48kHz restrictions.
  - Integrate with `codec_registry`:
    - Open any file from VFS (`/DEMO1.WAV`, `/MUSIC/TEST.MP3`, `/MUSIC/TEST.FLAC`, etc.).
    - Detect codec type, instantiate decoder, extract stream metadata (artist, title, album, bitrate, duration).
    - Decode frames sequentially in worker loop, submit to `audio_stream_write()`, and pipe to mixer.

---

### 3.6 Container Demuxer & BOSpectra Bridge (`kernel/media/bospectra/`)
- **[MODIFY] `kernel/media/bospectra/audio/bridge/audio_bridge.c`**:
  - Connect `bospectra_audio_bridge_write_pcm` to the active kernel audio stream instead of discarding PCM bytes.
- **[MODIFY] `kernel/media/bospectra/container/mp4/mp4_parser.c`**:
  - Connect MP4 Track 1 (`mp4a`) to the AAC decoder.

---

### 3.7 Build System Integration (`build.ps1`)
- **[MODIFY] `build.ps1`**:
  - Add clang freestanding compilation rules for:
    - `kernel/audio/codecs/*.c`
    - `kernel/audio/mixer/audio_resampler.c`
    - `kernel/audio/mixer/audio_channel.c`
  - Link new object files into `kernel.bin`.

---

## 4. Test Matrix & Automated Verification Harness

### 4.1 Real Audio Corpus Provisioning
Provision genuine test audio assets into FAT32 ESP partition via `gpt_image_builder`:
1. **WAV Corpus**:
   - `DEMO1.WAV` (48 kHz, 16-bit, Stereo)
   - `TEST_44K.WAV` (44.1 kHz, 16-bit, Stereo)
   - `TEST_MONO.WAV` (44.1 kHz, 16-bit, Mono)
   - `TEST_24B.WAV` (96 kHz, 24-bit, Stereo)
2. **MP3 Corpus**:
   - `TEST_CBR.MP3` (44.1 kHz, 128 kbps CBR, ID3v2 tag)
   - `TEST_VBR.MP3` (44.1 kHz, VBR, ID3v2 tag)
3. **FLAC Corpus**:
   - `TEST_16B.FLAC` (44.1 kHz, 16-bit, Stereo)
   - `TEST_24B.FLAC` (96 kHz, 24-bit, Stereo)
4. **AAC / MP4 Corpus**:
   - `TEST.MP4` (Track 1 AAC-LC audio, 44.1 kHz Stereo)
5. **Vorbis Corpus**:
   - `TEST.OGG` (44.1 kHz, Stereo Ogg Vorbis)

### 4.2 Automated Verification Script
- **`tools/verify_phase_m2_audio.py`**:
  - Boots QEMU in pure UEFI mode with Intel HDA (`-device intel-hda -device hda-duplex`).
  - Verifies AC97 regression (`-device AC97`).
  - Verifies serial telemetry checklist:
    ```text
    [CODEC] WAV initialized      [PASS]
    [CODEC] MP3 initialized      [PASS]
    [CODEC] FLAC initialized     [PASS]
    [CODEC] AAC initialized      [PASS]
    [CODEC] Vorbis initialized   [PASS]
    [RESAMPLER] 44100 -> 48000   [PASS]
    [CHANNEL] Mono -> Stereo     [PASS]
    [MIXER] Stream mixed         [PASS]
    [HDA] DMA stream progressing [PASS]
    [SYSCALL] User audio write   [PASS]
    [AUDIO] Playback verified    [PASS]
    ```

---

## 5. Rollback & Phase Isolation Boundaries

- **Rollback Safety**: All changes build on top of existing abstractions. If any codec encounters instability, it can be disabled in `codec_registry.c` without impacting WAV or hardware HAL drivers.
- **Rule 0 Compliance**: Source code modifications will begin strictly in Task 3 upon user review and approval of this plan.
