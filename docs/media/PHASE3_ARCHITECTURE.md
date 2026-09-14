# ATOMS OS — Phase 3 VFS + Audio Userspace Media Bridges Architecture
**Subsystem:** Userspace Media Engine ➔ Production VFS & Audio Bridges  
**Milestone:** Phase 3 Architecture Specification  
**Date:** September 12, 2026  
**Status:** **APPROVED ARCHITECTURE SPECIFICATION (RULE 0 ENFORCED)**  

---

## 1. Architectural Topology & Ring-3 Boundary

```text
                             RING 3 (USERSPACE)
┌────────────────────────────────────────────────────────────────────────┐
│                           media_player.elf                             │
│                     (UI Canvas / Controls / Events)                    │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │
                                    ▼
                         BOS Media API / Pipeline
                                    │
               ┌────────────────────┴────────────────────┐
               ▼                                         ▼
        BOSMediaStream                            BOSAudioStream
    ┌──────────────────────┐                  ┌──────────────────────┐
    │  - 64 KB Read Cache  │                  │  - Format Negotiator │
    │  - Cache Hit/Miss    │                  │  - Bounded FIFO      │
    │  - Signed Seek Calc  │                  │  - S16 / FLTP Conv   │
    │  - EOF / Error State │                  │  - Backpressure      │
    └──────────┬───────────┘                  └──────────┬───────────┘
               │                                         │
               ▼                                         ▼
          AVIO Bridge                               Audio User API
       (Custom Callback)                        (audio_stream_* calls)
               │                                         │
═══════════════╪═════════════════════════════════════════╪════════════════ SYSCALL BOUNDARY
               │ SYS_OPEN / SYS_READ                     │ SYS_AUDIO_CALL (43U)
               │ SYS_SEEK / SYS_CLOSE                    │ (with pointer validation)
               ▼                                         ▼
       Kernel VFS Core                            BOS Audio HAL
   (vfs_open, vfs_read,                       (audio_core, audio_mixer,
    vfs_seek, vfs_close)                       driver registry)
               │                                         │
       ┌───────┴───────┐                         ┌───────┴───────┐
       ▼               ▼                         ▼               ▼
   BOFS Volume     FAT32 / USB               Intel HDA        AC97 Driver
   (Transactional   (USB MSC /               (DMA Buffers     (PCI Legacy
    WAL Inodes)      RAMDisk)                 Streams)         Hardware)
```

---

## 2. BOS Media Stream Architecture (`BOSMediaStream`)

### 2.1 Interface Definition
The native stream abstraction decouples the demuxers from host operating system APIs while guaranteeing safe, buffered access to the ATOMS VFS:

```c
typedef struct BOSMediaStream {
    int      fd;
    uint64_t size;
    uint64_t position;
    bool     eof;
    int      error_code;
    
    // 64 KB Read Cache
    uint8_t  cache[65536];
    uint64_t cache_offset;
    size_t   cache_valid_bytes;
    size_t   cache_pos;

    // Telemetry Statistics
    uint64_t stat_cache_hits;
    uint64_t stat_cache_misses;
    uint64_t stat_refills;
    uint64_t stat_bytes_read;
    uint64_t stat_syscalls;
    
    // Function Pointers
    int      (*read)(struct BOSMediaStream* s, void* buffer, size_t count);
    int      (*seek)(struct BOSMediaStream* s, int64_t offset, int whence);
    int64_t  (*tell)(struct BOSMediaStream* s);
    uint64_t (*size_fn)(struct BOSMediaStream* s);
    bool     (*is_eof)(struct BOSMediaStream* s);
    int      (*get_error)(struct BOSMediaStream* s);
    void     (*close)(struct BOSMediaStream* s);
} BOSMediaStream;
```

### 2.2 Buffering & Cache Hit/Miss Logic
1. **Cache Hit:** If the requested range `[position, position + count)` is entirely within `[cache_offset, cache_offset + cache_valid_bytes)`, data is copied directly from `cache` with zero syscalls.
   - Increment `stat_cache_hits` and emit `[MEDIA-P3] VFS_CACHE_HIT`.
2. **Cache Miss & Refill:** If requested bytes lie outside the cache:
   - For reads smaller than 32 KB: Refill the full 64 KB cache starting at the target position via a single `SYS_READ`. Increment `stat_refills` and emit `[MEDIA-P3] VFS_REFILL`.
   - For bulk reads $\ge 32\text{ KB}$: Bypass the cache and read directly into the caller's buffer to prevent double-copying.
3. **Seek Invalidation:** On seek, if the new position remains within the current cache window, adjust `cache_pos` without discarding cache; otherwise invalidate the cache.

---

## 3. AVIO Bridge Integration (`BOSMediaAVIO`)

To ensure standard compliance with upstream demuxers (including FFmpeg `libavformat`), a standard custom I/O bridge wraps `BOSMediaStream`:

```c
typedef struct BOSMediaAVIO {
    BOSMediaStream* stream;
    void*           opaque;
    int           (*read_packet)(void* opaque, uint8_t* buf, int buf_size);
    int64_t       (*seek)(void* opaque, int64_t offset, int whence);
} BOSMediaAVIO;
```

### Protocol & Telemetry Markers:
- `AVIO_CREATE`: Stream binding verified.
- `AVIO_READ`: Invokes `stream->read()`; records byte count.
- `AVIO_SEEK`: Invokes `stream->seek()`; records resulting offset.
- `AVIO_EOF`: Propagated when `read()` returns 0 at or beyond stream end.
- `AVIO_ERROR`: Propagated when `read()` or `seek()` returns negative code.
- `AVIO_DESTROY`: Closes stream and cleans up context.

---

## 4. BOS Audio Stream Architecture (`BOSAudioStream`)

### 4.1 Interface Specification
```c
typedef struct BOSAudioStream {
    uint32_t         stream_id;
    AudioPcmFormat   format;
    bool             is_playing;
    bool             is_configured;
    
    // Bounded Audio FIFO (128 KB buffer = ~740ms of 44.1kHz 16-bit stereo)
    uint8_t*         fifo_buffer;
    size_t           fifo_capacity;
    size_t           fifo_write_pos;
    size_t           fifo_read_pos;
    size_t           fifo_queued_bytes;
    
    // Statistics & Backpressure
    uint32_t         stat_underruns;
    uint32_t         stat_overruns;
    uint32_t         stat_backpressures;
    uint64_t         stat_total_frames;
} BOSAudioStream;

BOSAudioStream* bos_audio_stream_create(void);
int             bos_audio_stream_configure(BOSAudioStream* s, uint32_t sample_rate, uint8_t channels, uint8_t bit_depth);
int             bos_audio_stream_start(BOSAudioStream* s);
int             bos_audio_stream_write(BOSAudioStream* s, const void* pcm_data, size_t size_bytes);
int             bos_audio_stream_pause(BOSAudioStream* s);
int             bos_audio_stream_resume(BOSAudioStream* s);
int             bos_audio_stream_drain(BOSAudioStream* s);
int             bos_audio_stream_flush(BOSAudioStream* s);
int             bos_audio_stream_stop(BOSAudioStream* s);
void            bos_audio_stream_destroy(BOSAudioStream* s);
```

### 4.2 Audio Format Negotiation
- **Sample Rate:** Probe Audio HAL capability. If 44,100 Hz, 48,000 Hz, or 22,050 Hz is requested, configure hardware mixer directly.
- **Sample Format Conversion:**
  - Input `PCM_FORMAT_S16_LE`: Pass-through directly.
  - Input `FLTP` (32-bit Float Planar): Clamp $[-1.0, 1.0] \to [-32768, 32767]$ and interleave left/right channels.
  - Input `S32_LE` (32-bit Integer): Right-shift 16 bits with saturation.
- **Channels:** Convert mono to dual-mono (stereo duplicate) if backend requires 2 channels.

### 4.3 PCM Buffer Ownership Model
To eliminate double-free, use-after-free, and memory corruption:

```text
[DECODER BUFFER]  --> Decodes raw stream chunk into local decoder memory
       │
       ▼ (Pass to conversion)
[CONVERSION BUFFER] -> Converts FLTP/S32 to S16_LE interleaved
       │
       ▼ (Enqueue)
[BOUNDED FIFO]    --> Owned by BOSAudioStream queue
       │
       ▼ (Stream Write via SYS_AUDIO_CALL)
[AUDIO HAL RING]  --> Kernel HDA/AC97 DMA consumes directly from userspace packet
       │
       ▼ (Interrupt Handled)
[RELEASED]        --> Queue read index advances; bytes marked available
```

---

## 5. Kernel Syscall Safety & Memory Protection

### 5.1 Pointer Validation on `SYS_AUDIO_CALL` (Op `STREAM_WRITE`)
In `kernel/core/syscall/src/services.c`:
```c
case ATOMS_AUDIO_OP_STREAM_WRITE: {
    if (!a2) return 0;
    if (!syscall_validate_user_ptr((const void*)a2, sizeof(AudioPcmPacket))) {
        return 0;
    }
    const AudioPcmPacket* pkt = (const AudioPcmPacket*)a2;
    if (!pkt->pcm_data || pkt->size_bytes == 0 || pkt->size_bytes > 262144) return 0;
    if (!syscall_validate_user_ptr((const void*)pkt->pcm_data, pkt->size_bytes)) {
        return 0;
    }
    return (uint64_t)audio_stream_write((uint32_t)a1, pkt);
}
```

### 5.2 Signed Offset Arithmetic in `vfs_seek()`
In `kernel/vfs/vfs_legacy/src/vfs.c`:
```c
int vfs_seek(int fd, int64_t offset, int whence) {
    if (fd < 3 || fd >= MAX_OPEN_FILES || !g_fd_table[fd].in_use) return -1;
    VFS_Node* node = g_fd_table[fd].node;
    uint64_t file_size = node ? node->size : 0;
    int64_t target = 0;

    if (whence == 0) { // SEEK_SET
        target = offset;
    } else if (whence == 1) { // SEEK_CUR
        target = (int64_t)g_fd_table[fd].offset + offset;
    } else if (whence == 2) { // SEEK_END
        target = (int64_t)file_size + offset;
    } else {
        return -1;
    }

    if (target < 0) target = 0;
    g_fd_table[fd].offset = (uint64_t)target;
    return (int)g_fd_table[fd].offset;
}
```

### 5.3 Process Teardown Resource Cleanup
On `sys_service_exit(code)` / `ATOMS_Process_Terminate(pid)`:
1. Close all open VFS descriptors owned by process.
2. Destroy all Audio HAL streams associated with `pid` (`audio_core_destroy_streams_by_pid(pid)`).
3. Unmap all BOSurface buffers.
4. Release user page tables safely.
