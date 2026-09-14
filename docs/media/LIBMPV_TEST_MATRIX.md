# ATOMS OS — Native Media Engine Format Test Matrix & Certification Plan
**Document ID:** `docs/media/LIBMPV_TEST_MATRIX.md`  
**Subsystem:** ATOMS Native Media Engine (`libbos_media` / `libmpv`)  
**Status:** FORENSIC SPECIFICATION & CERTIFICATION MATRIX  
**Date:** September 12, 2026  

---

## 1. Certification Status Definitions

In strict accordance with the ATOMS Engineering Protocol, status labels are restricted to:
- `IMPLEMENTED`: Code complete and compiled.
- `HOST-VALIDATED`: Tested on build host / offline test harness.
- `QEMU-VALIDATED`: Verified inside pure UEFI QEMU environment.
- `REAL-HARDWARE-VALIDATED`: Tested and verified on physical Intel Core i3-14100F + RTX 4060 + H81 motherboard.
- `PARTIAL`: Partially functional or functional with known limitations.
- `NOT-IMPLEMENTED`: Planned but not yet integrated.
- `BLOCKED`: Blocked by external hardware/driver dependency.

---

## 2. Container & Codec Verification Matrix

### 2.1 Video Streams

| Codec | Profile / Level | Container(s) | Primary Test File | Decode Backend | Verification Criteria | Target Status |
|:---|:---|:---|:---|:---|:---|:---:|
| **H.264 / AVC** | Baseline (L3.0/3.1) | MP4, MKV | `TEST_H264_BASELINE.MP4` | SW (FFmpeg) / HW | Bitstream decode, correct aspect ratio, CRC match | **QEMU & REAL HARDWARE** |
| **H.264 / AVC** | Main Profile (L4.0) | MP4, TS | `TEST_H264_MAIN.MP4` | SW (FFmpeg) / HW | CABAC entropy, B-frames, correct PTS ordering | **QEMU & REAL HARDWARE** |
| **H.264 / AVC** | High Profile (L4.1/4.2) | MP4, MKV | `Dolby_Vision_AtmosHDR.mp4` | SW (FFmpeg) / HW | 8x8 transform, 1080p @ 24/60fps, zero black frame | **QEMU & REAL HARDWARE** |
| **HEVC / H.265**| Main / Main 10 | MP4, MKV | `TEST_HEVC.MP4` | SW (FFmpeg) / HW | CTU 64x64 decode, 10-bit to 8-bit dither / ARGB | **QEMU & REAL HARDWARE** |
| **VP8** | Standard | WebM | `TEST_VP8.WEBM` | SW (FFmpeg) | WebM EBML demux, YUV420P conversion | **QEMU & REAL HARDWARE** |
| **VP9** | Profile 0 / 2 | WebM, MKV | `TEST_VP9.WEBM` | SW (FFmpeg) | Superframe demux, 1080p presentation | **QEMU & REAL HARDWARE** |
| **AV1** | Main Profile | MP4, WebM | `TEST_AV1.WEBM` | `dav1d` / SW | OBU packet parse, smooth frame delivery | **QEMU & REAL HARDWARE** |
| **MPEG-2** | Main Profile | TS, MPG | `TEST_MPEG2.TS` | SW (FFmpeg) | Interlaced deinterlace/weave, 1080i/720p | **QEMU & REAL HARDWARE** |
| **MJPEG** | Baseline | AVI | `DOLBY.AVI` | SW (FFmpeg) | Frame-by-frame JPEG quantization, legacy compat | **QEMU & REAL HARDWARE** |

### 2.2 Audio Streams

| Codec | Sample Rates | Channels | Test File | Audio HAL Target | Verification Criteria | Target Status |
|:---|:---|:---|:---|:---|:---|:---:|
| **MP3** | 44.1kHz, 48kHz | Stereo | `NCSJanjiHeroesTonight.mp3` | Intel HDA / AC97 | Frame header sync, 48kHz resample, zero underrun | **REAL HARDWARE** |
| **WAV** | 44.1kHz, 48kHz, 96kHz | Stereo | `TEST.WAV` | Intel HDA / AC97 | Direct uncompressed PCM streaming, zero latency | **REAL HARDWARE** |
| **FLAC** | 48kHz, 96kHz (16/24-bit)| Stereo | `TEST.FLAC` | Intel HDA / AC97 | Lossless unpack, 24-to-16 bit conversion | **REAL HARDWARE** |
| **AAC** | 44.1kHz, 48kHz | Stereo | `TEST_AUDIO.AAC` | Intel HDA / AC97 | ADTS frame parse, LC profile decode | **REAL HARDWARE** |
| **Opus** | 48kHz | Stereo | `TEST_AUDIO.OPUS` | Intel HDA / AC97 | Low-delay frame decode, packet loss recovery | **REAL HARDWARE** |
| **Vorbis** | 44.1kHz, 48kHz | Stereo | `TEST_AUDIO.OGG` | Intel HDA / AC97 | Ogg page demux, Vorbis floor/residue decode | **REAL HARDWARE** |

### 2.3 Subtitles & Metadata

| Type | Format | Verification Criteria | Target Status |
|:---|:---|:---|:---:|
| **Text Subtitles** | SRT (SubRip) | Accurate timestamp display over video canvas | **IMPLEMENTED** |
| **Styled Subtitles** | ASS / SSA (`libass`) | Font rendering, text positioning, styling | **IMPLEMENTED** |
| **Embedded Subtitles**| MKV subtitle track | Track selection via BOS Media API | **IMPLEMENTED** |
| **ID3v2 / MP4 Tags** | Title, Artist, Album | Displayed in "Now Playing" toolbar | **REAL HARDWARE** |

---

## 3. Strict Verification & Forensic Rules

1. **No Fake Success / No Moving Gradients:**  
   Under no circumstances will a color-gradient generator or synthetic counter be substituted for actual decoded pixel buffers. If decode fails, the player reports `BOS_MEDIA_STATE_ERROR` with stage telemetry (`FIRST_FAILURE=...`).
2. **Decoded Frame Proof:**  
   Every verification capture must compute a 32-bit CRC or hash of:
   - Frame 0 (First presented frame)
   - Frame 1
   - Frame 10
   - Frame 50
3. **A/V Drift Tolerance:**  
   Telemetry records:
   $$\Delta_{\text{drift}} = |\text{PTS}_{\text{audio}} - \text{PTS}_{\text{video}}|$$
   Maximum allowable drift during continuous playback is $\le 25\text{ ms}$.
4. **Seeking Verification:**  
   Must perform sequential seeks across media: $0\% \to 25\% \to 50\% \to 75\% \to 10\% \to 90\% \to 100\%$. The pipeline must flush buffers, clear DPB references, seek the demuxer, and resume playback within $\le 200\text{ ms}$ without crashing or freezing.
