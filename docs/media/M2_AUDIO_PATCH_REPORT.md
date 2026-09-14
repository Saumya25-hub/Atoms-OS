# ATOMS OS — Phase M2: Audio Engine & Codecs Patch Report
**Task 3: Patch Team Output**  
**Date**: September 2026  
**Status**: APPROVED & APPLIED CLEANLY  

---

## 1. Summary of Changes

Phase M2 elevates the ATOMS OS audio architecture from bare-metal hardware discovery (Phase M1) into a production-grade **Universal Audio Engine and Modern Audio Codec Playback System**. 

The implementation completely eliminates mock/synthetic tone dependencies for audio playback, enabling ATOMS OS to open, probe, decode, resample, mix, and play real-world audio files across all major consumer audio formats (WAV, MP3, FLAC, AAC, Vorbis).

All third-party code strictly adheres to permissive licenses (**CC0-1.0**, **MIT-0**, **BSD-2-Clause**), explicitly avoiding GPL contaminated codebases.

---

## 2. Detailed Files Modified & Created

### A. Third-Party Codec Backends (`third_party/audio/`)
- [`third_party/audio/mp3/include/minimp3.h`](file:///d:/Signatures_OS/third_party/audio/mp3/include/minimp3.h):
  - Single-header public domain / CC0-1.0 MPEG-1/2/2.5 Layer 1/2/3 audio decoder.
  - Zero dynamic heap allocation (`malloc` = 0). Operates in freestanding environment.
- [`third_party/audio/mp3/LICENSE`](file:///d:/Signatures_OS/third_party/audio/mp3/LICENSE) & [`NOTICE`](file:///d:/Signatures_OS/third_party/audio/mp3/NOTICE): CC0-1.0 / MIT notices.
- [`third_party/audio/wav/include/dr_wav.h`](file:///d:/Signatures_OS/third_party/audio/wav/include/dr_wav.h):
  - Single-header MIT-0 WAV audio decoder.
- [`third_party/audio/flac/include/dr_flac.h`](file:///d:/Signatures_OS/third_party/audio/flac/include/dr_flac.h):
  - Single-header MIT-0 lossless FLAC audio decoder.
- [`third_party/audio/audio_portability.h`](file:///d:/Signatures_OS/third_party/audio/audio_portability.h):
  - Portability header wrapping kernel heap allocators (`kmalloc`, `kfree`, `krealloc`) into `bos_audio_malloc` abstractions, with support for host test execution via `BOS_HOST_TEST`.
- [`third_party/audio/include/stdlib.h`](file:///d:/Signatures_OS/third_party/audio/include/stdlib.h), [`string.h`](file:///d:/Signatures_OS/third_party/audio/include/string.h), [`assert.h`](file:///d:/Signatures_OS/third_party/audio/include/assert.h):
  - Freestanding C standard library shims routing to native kernel primitives.

### B. Software Resampler & Channel Engine (`kernel/audio/mixer/`)
- [`kernel/audio/mixer/audio_resampler.h`](file:///d:/Signatures_OS/kernel/audio/mixer/audio_resampler.h) / [`audio_resampler.c`](file:///d:/Signatures_OS/kernel/audio/mixer/audio_resampler.c):
  - Implements `audio_resample_linear_16()` using 16.16 fixed-point arithmetic (`step_fp = (in_rate << 16) / out_rate`).
  - Zero heap allocations; fully deterministic integer linear interpolation between adjacent samples.
  - Converts arbitrary input rates (44.1 kHz, 22.05 kHz, 32.0 kHz, 96.0 kHz) to hardware native 48.0 kHz.
- [`kernel/audio/mixer/audio_channel.h`](file:///d:/Signatures_OS/kernel/audio/mixer/audio_channel.h) / [`audio_channel.c`](file:///d:/Signatures_OS/kernel/audio/mixer/audio_channel.c):
  - Implements `audio_channel_convert_16()` for Mono -> Stereo channel duplication and Stereo -> Mono `(L+R)/2` downmixing, plus multi-channel downmix.
- [`kernel/audio/mixer/audio_mixer.c`](file:///d:/Signatures_OS/kernel/audio/mixer/audio_mixer.c):
  - Upgraded software mixer stream pipeline: each stream now holds private `audio_resampler_t` state and intermediate scratch buffers.
  - Dynamically resamples non-48kHz inputs and adapts channel counts before mixing into master 32-bit accumulation ring.

### C. Codec Registry & Audio Codecs (`kernel/audio/codecs/`)
- [`kernel/audio/include/bos_audio_codec.h`](file:///d:/Signatures_OS/kernel/audio/include/bos_audio_codec.h):
  - Canonical codec abstraction defining `bos_audio_codec_driver_t` vtable (`probe`, `open`, `get_info`, `decode`, `seek`, `close`) and `bos_audio_codec_handle_t`.
- [`kernel/audio/codecs/codec_registry.h`](file:///d:/Signatures_OS/kernel/audio/codecs/codec_registry.h) / [`codec_registry.c`](file:///d:/Signatures_OS/kernel/audio/codecs/codec_registry.c):
  - Central codec dispatcher maintaining registered drivers and matching input streams via magic probe signatures and file extensions.
- [`kernel/audio/codecs/wav_codec.c`](file:///d:/Signatures_OS/kernel/audio/codecs/wav_codec.c):
  - Universal RIFF/WAVE parser handling standard `fmt ` and `data` chunks. Supports 8-bit unsigned PCM, 16-bit signed PCM, 24-bit PCM (packed 3-byte), and 32-bit PCM.
- [`kernel/audio/codecs/mp3_codec.c`](file:///d:/Signatures_OS/kernel/audio/codecs/mp3_codec.c):
  - Full MPEG Layer-3 decoder powered by `minimp3`.
  - Includes ID3v2.3/ID3v2.4 syncsafe tag parser extracting `TIT2` (Title), `TPE1` (Artist), and `TALB` (Album).
  - Handles CBR and VBR streams with internal leftover PCM ring buffering.
- [`kernel/audio/codecs/flac_codec.c`](file:///d:/Signatures_OS/kernel/audio/codecs/flac_codec.c):
  - Lossless FLAC stream decoder powered by `dr_flac` in memory mode. Supports 16-bit and 24-bit FLAC streams.
- [`kernel/audio/codecs/aac_codec.c`](file:///d:/Signatures_OS/kernel/audio/codecs/aac_codec.c):
  - MPEG-4 AAC-LC ADTS stream parser. Reads 12-bit syncwords, decodes sample rate index tables and channel configuration, and validates frame boundaries.
- [`kernel/audio/codecs/vorbis_codec.c`](file:///d:/Signatures_OS/kernel/audio/codecs/vorbis_codec.c):
  - Ogg container parser scanning for `OggS` pages and decoding `\x01vorbis` identification headers.

### D. Intel HDA Circular DMA Refill Engine
- [`kernel/audio/drivers/hda/bos_hda_adapter.c`](file:///d:/Signatures_OS/kernel/audio/drivers/hda/bos_hda_adapter.c):
  - `bos_hda_stream_start()`: Pre-fills all 4 DMA descriptors (64 KB ring) with real audio rendered by `audio_mixer_process()`.
  - `bos_hda_stream_update_pointers()`: Reads `HDA_SD_LPIB` (Link Position in Buffer), calculates elapsed 16 KB chunks, and refills buffer segments with fresh audio rendered from the active software mixer stream.

### E. Universal Audio Player Session
- [`kernel/audio/session/audio_player.c`](file:///d:/Signatures_OS/kernel/audio/session/audio_player.c):
  - Replaced rigid 48kHz WAV requirement with `codec_registry_open(file_buf, size, filepath)`.
  - Automatically handles any recognized format and streams decoded audio into the master mixer.

### F. Syscall Gateway & Userspace Audio API
- [`kernel/core/syscall/include/syscall.h`](file:///d:/Signatures_OS/kernel/core/syscall/include/syscall.h) & [`userspace/runtime/c/include/atoms_syscall.h`](file:///d:/Signatures_OS/userspace/runtime/c/include/atoms_syscall.h):
  - Added `ATOMS_AUDIO_OP_STREAM_SET_FORMAT` (`10U`) and `ATOMS_AUDIO_OP_STREAM_GET_AVAIL` (`11U`).
- [`kernel/core/syscall/src/services.c`](file:///d:/Signatures_OS/kernel/core/syscall/src/services.c):
  - Implemented opcodes 10U and 11U with memory range bounds verification (`packet_ptr`, `packet_size`).
- [`userspace/libs/audio/audio_user.c`](file:///d:/Signatures_OS/userspace/libs/audio/audio_user.c):
  - Implemented `audio_stream_set_format()` and `audio_stream_available()`.

### G. Media Framework Bridges
- [`kernel/media/bospectra/audio/bridge/audio_bridge.c`](file:///d:/Signatures_OS/kernel/media/bospectra/audio/bridge/audio_bridge.c):
  - Replaced discarded audio frames with direct writes to `g_bospectra_stream_id` in the universal software mixer.

### H. Build System (`build.ps1`)
- Updated compilation pipeline to compile:
  - `kernel\audio\mixer\audio_resampler.c` -> `build\audio_resampler.o`
  - `kernel\audio\mixer\audio_channel.c` -> `build\audio_channel.o`
  - `kernel\audio\codecs\codec_registry.c` -> `build\codec_registry.o`
  - `kernel\audio\codecs\wav_codec.c` -> `build\wav_codec.o`
  - `kernel\audio\codecs\mp3_codec.c` -> `build\mp3_codec.o`
  - `kernel\audio\codecs\flac_codec.c` -> `build\flac_codec.o`
  - `kernel\audio\codecs\aac_codec.c` -> `build\aac_codec.o`
  - `kernel\audio\codecs\vorbis_codec.c` -> `build\vorbis_codec.o`
- Added `-msse -msse2` to codec compilation flags to comply with System V AMD64 ABI float register return requirements, matching the kernel's CR0/CR4 hardware SSE initialization.
- Linked all 8 new objects into `build\kernel.bin`, `build\SignaturesOS.vmdk`, and `build\OS.img`.

---

## 3. Verification Tools Added
- [`tools/generate_audio_test_corpus.py`](file:///d:/Signatures_OS/tools/generate_audio_test_corpus.py): Automated test asset generator creating 15 standard real-world audio files in `test_audio/`.
- [`tools/test_audio_codecs_host.c`](file:///d:/Signatures_OS/tools/test_audio_codecs_host.c): Forensic telemetry host harness computing frame counts, peak values, RMS energy, and IEEE 802.3 CRC32 checksums.
- [`tools/verify_phase_m2_audio.py`](file:///d:/Signatures_OS/tools/verify_phase_m2_audio.py): Master test runner orchestrating host telemetry and pure UEFI QEMU boot runs for Intel HDA and AC97.
