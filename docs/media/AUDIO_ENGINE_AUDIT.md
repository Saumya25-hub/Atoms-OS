# ATOMS OS — AUDIO ENGINE & HARDWARE FORENSIC AUDIT
**Document ID**: `docs/media/AUDIO_ENGINE_AUDIT.md`  
**Subsystem**: Audio HAL, Hardware Drivers, Audio Core, Mixer, Decoders, Userspace Bridge  
**Date**: September 10, 2026  
**Status**: AUDIT COMPLETE  

---

## 1. Executive Summary

A forensic inspection of the audio subsystem across `kernel/audio/`, `kernel/audio_v3/`, `userspace/libs/audio/`, and `kernel/core/syscall/` reveals that while internal kernel ring buffers and software mixing exist, **native audio playback on physical hardware is non-functional** and **userspace audio is completely disconnected**.

Specifically:
1. **Target Hardware Incompatibility**: Physical H81 motherboards (Intel Haswell LGA1150) feature an Intel Lynx Point High Definition Audio (HDA) controller. The OS only implements an AC97 PCI driver. The driver registration code attempts to probe HDA devices as AC97, encounters memory-mapped BARs instead of I/O BARs, and aborts initialization.
2. **Missing Userspace-to-Kernel Audio Syscalls**: `userspace/libs/audio/audio_user.c` contains dummy stubs that discard all PCM writes. There are zero audio syscalls in `userspace/runtime/c/include/atoms_syscall.h`.
3. **Format & Decoder Absence**: Zero compressed audio decoders exist (no MP3, FLAC, AAC, OGG, or Opus). The only audio container parsed is uncompressed WAV, but it is locked to a single rigid format (48 kHz, 16-bit, stereo PCM).
4. **Diagnostic Preload Limitation**: `kernel/audio/session/audio_player.c` was modified during earlier debugging to preload a maximum of 6.14 MB into heap and immediately close the file handle, breaking standard streaming from storage.

---

## 2. Format Support Matrix

| Format | Container | Decoder Exists? | Native Code? | Playback Works? | Seeking? | Metadata? | Actual Forensic Status |
|---|---|---|---|---|---|---|---|
| **WAV (PCM)** | RIFF / WAV | **YES** (Header parser) | **YES** | **PARTIAL** (QEMU AC97 only; 48kHz/16-bit/2ch only) | **NO** (Loop only) | **NO** | Rigidly restricted; rejects all other sample rates; RAM-only 6MB limit |
| **MP3** | MPEG-1 Audio / ID3 | **NO** | **NO** | **NO** | **NO** | **NO** | **MISSING** (No bitstream parser, Huffman tables, or synthesis filterbank) |
| **FLAC** | FLAC / Ogg | **NO** | **NO** | **NO** | **NO** | **NO** | **MISSING** (No FLAC subframe decoder, LPC, or Rice entropy decoding) |
| **AAC** | ADTS / MP4 | **NO** | **NO** | **NO** | **NO** | **NO** | **MISSING** (No MDCT, spectral noiseless coding, or filterbanks) |
| **OGG / Vorbis** | OGG | **NO** | **NO** | **NO** | **NO** | **NO** | **MISSING** (No Ogg page parser or Vorbis floor/residue decoder) |
| **Opus** | OGG / WebM | **NO** | **NO** | **NO** | **NO** | **NO** | **MISSING** (No SILK or CELT decoders) |
| **Synthetic PCM** | Raw Buffer | **YES** | **YES** | **YES** (Kernel only) | N/A | N/A | Generates sine, square, saw, and noise waveforms (`audio_pcm.c`) |

---

## 3. Audio Hardware & Driver Layer Audit

### 3.1 Physical Hardware Reality vs Kernel Driver
- **Physical Target Hardware**:
  - Motherboard: H81 Chipset (LGA1150 Haswell)
  - Audio Controller: Intel Lynx Point PCH High Definition Audio (Vendor: `0x8086`, Device: `0x8C20` or similar, PCI Class: `0x04`, Subclass: `0x03`).
- **Kernel Audio Driver Code**:
  - Implemented in [`kernel/audio/drivers/ac97/ac97.c`](file:///d:/Signatures_OS/kernel/audio/drivers/ac97/ac97.c).
  - Designed exclusively for Intel 82801AA/AB/BA/CA/DB AC97 controllers (PCI Class: `0x04`, Subclass: `0x01`).
- **Probing Flaw**:
  In [`kernel/audio/hal/audio_driver_registry.c:30`](file:///d:/Signatures_OS/kernel/audio/hal/audio_driver_registry.c#L30):
  ```c
  PCIDevice dev;
  bool found = pci_find_by_class(0x04, 0x01, &dev) || pci_find_by_class(0x04, 0x03, &dev);
  ```
  The registry finds the H81 HDA controller (`0x04, 0x03`), but iterates `g_registered_drivers`, where **only** `ac97_driver` is registered.
  In `ac97_hal_init()` ([ac97.c:18-24](file:///d:/Signatures_OS/kernel/audio/drivers/ac97/ac97.c#L18-L24)):
  ```c
  uint32_t bar0 = pci_read_config(target_bus, target_slot, 0, 0x10);
  uint32_t bar1 = pci_read_config(target_bus, target_slot, 0, 0x14);
  
  if (!(bar0 & 1) || !(bar1 & 1)) {
      display_print("[AC97] FAILED: BARs are not I/O mapped.\n");
      return false;
  }
  ```
  Intel HDA uses 64-bit Memory-Mapped I/O (MMIO), so bit 0 of BAR0 is `0`. The check fails immediately.
  **Verdict**: **NO NATIVE AUDIO OUTPUT DRIVER FOUND FOR H81 HARDWARE.**

### 3.2 AC97 DMA & Playback Engine (QEMU Emulation)
- When executed in QEMU with `-device AC97`, the driver operates via a 32-entry Buffer Descriptor List (BDL) using Scatter-Gather DMA.
- Monitored by `ac97_playback_update()` ([ac97_playback.c:340](file:///d:/Signatures_OS/kernel/audio/drivers/ac97/ac97_playback.c#L340)), which queries hardware registers `PO_CIV` (Current Index Value), `PO_LVI` (Last Valid Index), and `PO_SR` (Status Register).
- Underrun recovery (`DCH` bit) and circular ring refills are functional within QEMU.

---

## 4. Userspace Audio Architecture & Missing Syscall Bridge

### 4.1 Userspace Library (`userspace/libs/audio/audio_user.c`)
Inspection of `audio_user.c` reveals that **all APIs are no-op stubs**:
```c
void audio_init(void) {}
void audio_shutdown(void) {}
uint32_t audio_stream_create(uint32_t process_id) { return s_next_audio_stream++; }
bool audio_stream_destroy(uint32_t stream_id) { return true; }
bool audio_stream_pause(uint32_t stream_id) { return true; }
bool audio_stream_resume(uint32_t stream_id) { return true; }
bool audio_stream_stop(uint32_t stream_id) { return true; }
bool audio_stream_set_format(uint32_t stream_id, const AudioPcmFormat* format) { return true; }
size_t audio_stream_write(uint32_t stream_id, const AudioPcmPacket* packet) {
    if (!packet) return 0;
    return packet->frame_count; // DISCARDS DATA SILENTLY
}
size_t audio_stream_read(uint32_t stream_id, uint8_t* buffer, size_t size_bytes) { return 0; }
size_t audio_stream_available(uint32_t stream_id) { return 4096; }
size_t audio_stream_capacity(uint32_t stream_id) { return 65536; }
```

### 4.2 Syscall Gateway Inspection
In [`userspace/runtime/c/include/atoms_syscall.h`](file:///d:/Signatures_OS/userspace/runtime/c/include/atoms_syscall.h):
- Syscalls defined: `SYS_WRITE (0)` through `SYS_WRITE_FILE (29)`.
- **Zero audio syscalls exist**.
In `kernel/core/syscall/syscall_gateway.h`, line 28 defines `#define SYS_AUDIO_PLAY 12`, but `syscall_gateway.c` does not handle it in `ATOMS_KernelService_Dispatch()`, routing it directly to `default: return -1;`.

**Forensic Conclusion**: There is currently no mechanism for any userspace process to transmit audio data to the kernel.

---

## 5. Kernel Audio Player (`audio_player.c`) Limitations

The kernel-level player implementation in [`kernel/audio/session/audio_player.c`](file:///d:/Signatures_OS/kernel/audio/session/audio_player.c) exhibits critical constraints:

1. **Rigid Format Restriction** ([audio_player.c:184-188](file:///d:/Signatures_OS/kernel/audio/session/audio_player.c#L184-L188)):
   ```c
   if (fmt.audio_format != 1 || fmt.num_channels != 2 || fmt.sample_rate != 48000 || fmt.bits_per_sample != 16) {
       display_print("[AUDIO_PLAYER] Unsupported format (must be 48kHz 16-bit stereo PCM)!\n");
       vfs_close(fd);
       return;
   }
   ```
   Standard audio files encoded at 44.1 kHz (CD quality) or in mono are rejected.
2. **Diagnostic RAM Preload Hack** ([audio_player.c:226-244](file:///d:/Signatures_OS/kernel/audio/session/audio_player.c#L226-L244)):
   ```c
   g_RAM_Only_Test_Active = true;
   for (int i = 0; i < NUM_RAM_CHUNKS; i++) { // NUM_RAM_CHUNKS = 24, 256KB each = 6.14MB max
       g_ram_audio_chunks[i] = kmalloc(RAM_CHUNK_SIZE);
       int r = vfs_read(g_audio_session.fd, g_ram_audio_chunks[i], RAM_CHUNK_SIZE);
       ...
   }
   vfs_close(g_audio_session.fd);
   g_audio_session.fd = -1;
   ```
   The player preloads up to 6.14 MB into RAM, **closes the file handle**, and plays exclusively from RAM chunks. Any song longer than ~33 seconds (at 48kHz/16-bit stereo, 192 KB/sec) is abruptly truncated.
3. **No Seeking Support**: The player interface lacks `audio_player_seek()`. It only loops back to `data_offset` upon EOF.

---

## 6. Recommendations for Native Media Application

1. **Hardware Driver**: Implement a native Intel High Definition Audio (HDA) controller driver supporting Lynx Point/Haswell PCH (`PCI 0x04, 0x03`).
2. **Userspace Syscall Layer**: Define real audio system calls (`SYS_AUDIO_STREAM_CREATE`, `SYS_AUDIO_STREAM_WRITE`, `SYS_AUDIO_STREAM_CONTROL`) to bridge userspace applications with the kernel mixer.
3. **Standalone Software Decoders**: Port lightweight, freestanding, zero-dependency C decoders into userspace (e.g., `dr_wav` for arbitrary WAV PCM, `dr_mp3` or `minimp3` for MP3, and `dr_flac` for lossless audio).
4. **Resampling Engine**: Integrate a linear or polyphase resampler to convert arbitrary audio (44.1 kHz, 22.05 kHz) to the hardware's native output clock (48 kHz).
