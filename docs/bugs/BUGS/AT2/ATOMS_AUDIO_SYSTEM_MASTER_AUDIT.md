# ATOMS OS Audio System — Master Forensic Audit

**Audit Date:** July 16, 2026  
**Scope:** Current `HEAD` (`b96ad50`) — audit only, NO code modifications  
**Regression Under Investigation:** Audio plays correctly for ~2–3 seconds, then transitions to continuous noise/static ("shhhhh") and playback effectively stalls. Observed across QEMU, VirtualBox, and VMware.

---

## Table of Contents

1. [Architecture Overview Diagram](#1-architecture-overview-diagram)
2. [Stage 1: WAV File on Disk (FAT32)](#2-stage-1-wav-file-on-disk-fat32)
3. [Stage 2: VFS Layer](#3-stage-2-vfs-layer)
4. [Stage 3: FAT32 Driver — Cluster Walk & Disk Read](#4-stage-3-fat32-driver--cluster-walk--disk-read)
5. [Stage 4: ATA PIO Driver — Sector Transfer](#5-stage-4-ata-pio-driver--sector-transfer)
6. [Stage 5: Audio Player (Producer)](#6-stage-5-audio-player-producer)
7. [Stage 6: Ring Buffer (256 KB)](#7-stage-6-ring-buffer-256-kb)
8. [Stage 7: Audio Mixer](#8-stage-7-audio-mixer)
9. [Stage 8: AC97 DMA Buffer & BDL](#9-stage-8-ac97-dma-buffer--bdl)
10. [Stage 9: AC97 Playback Update (Consumer/IRQ)](#10-stage-9-ac97-playback-update-consumerirq)
11. [Stage 10: Hardware Registers & Physical Device](#11-stage-10-hardware-registers--physical-device)
12. [Initialization Order](#12-initialization-order)
13. [Scheduler & Task Model](#13-scheduler--task-model)
14. [Ring Buffer Watermark Strategy](#14-ring-buffer-watermark-strategy)
15. [Regression Analysis: Current vs. Last-Known-Good](#15-regression-analysis-current-vs-last-known-good)
16. [Root Cause Hypothesis for Current Regression](#16-root-cause-hypothesis-for-current-regression)
17. [Complete Call Graph (End-to-End)](#17-complete-call-graph-end-to-end)

---

## 1. Architecture Overview Diagram

```
┌──────────────────────────────────────────────────────────────────────────────┐
│                        ATOMS OS AUDIO PIPELINE                              │
│                                                                              │
│  ┌─────────┐    ┌─────────┐    ┌──────────┐    ┌──────────────┐             │
│  │ FAT32   │───>│ VFS     │───>│ ATA PIO  │───>│ audio_player │             │
│  │ DEMO1.  │    │ vfs_read│    │ rep insw │    │ _update()    │             │
│  │ WAV     │    │ fd table│    │ ata.c    │    │ 16KB chunks  │             │
│  └─────────┘    └─────────┘    └──────────┘    └──────┬───────┘             │
│                                                        │                     │
│                                                        ▼                     │
│                                              ┌──────────────────┐            │
│                                              │ 256 KB Ring Buf  │            │
│                                              │ AudioRingBuffer  │            │
│                                              │ head/tail SPSC   │            │
│                                              └────────┬─────────┘            │
│                                                        │                     │
│                                                        ▼                     │
│                                              ┌──────────────────┐            │
│                                              │ audio_mixer      │            │
│                                              │ _process()       │            │
│                                              │ Volume + Mix +   │            │
│                                              │ Normalize        │            │
│                                              └────────┬─────────┘            │
│                                                        │                     │
│                                                        ▼                     │
│                                              ┌──────────────────┐            │
│                                              │ 128 KB DMA Buf   │            │
│                                              │ 32 × BDL entries │            │
│                                              │ 4 KB per desc    │            │
│                                              └────────┬─────────┘            │
│                                                        │                     │
│                                                        ▼                     │
│           IRQ 0 @ 1000 Hz ──> ac97_playback_update() ──> CIV/LVI/SR regs    │
│                                                        │                     │
│                                                        ▼                     │
│                                              ┌──────────────────┐            │
│                                              │ AC97 Codec / DAC │            │
│                                              │ 48 kHz 16-bit    │
│                                              │ Stereo PCM       │            │
│                                              └──────────────────┘            │
└──────────────────────────────────────────────────────────────────────────────┘

PRODUCER THREAD: "AudioSvc" task, scheduler_sleep(20), calls audio_player_update()
CONSUMER: IRQ 0 timer_tick_handler @ 1000 Hz → audio_realtime_worker_pump() → ac97_playback_update()
```

---

## 2. Stage 1: WAV File on Disk (FAT32)

| Property | Value | Validation |
|---|---|---|
| **File** | `/DEMO1.WAV` (Normal Boot: `/BOOT1.WAV`) | `PROVEN FROM CODE` [desktop_shell.c:598](file:///D:/Signatures_OS/kernel/shell/desktop_shell/desktop_shell.c#L598) |
| **Size** | 39,933,194 bytes (`DEMO1.WAV`) / 1,035,370 bytes (`BOOT1.WAV`) | `PROVEN FROM FILESYSTEM` |
| **Format Required** | 48 kHz, 16-bit, 2-channel (Stereo), PCM (format=1) | `PROVEN FROM CODE` [audio_player.c:141](file:///D:/Signatures_OS/kernel/audio/session/audio_player.c#L141) |
| **Data Rate** | 48000 × 2 × 2 = **192,000 bytes/sec** | Derived |
| **Playback Duration** | DEMO1: ~208 sec, BOOT1: ~5.4 sec | Derived |
| **Disk Image Location** | Written by [image_builder.c](file:///D:/Signatures_OS/tools/image_builder.c) into FAT32 partition at cluster offsets `dir[17]` (BOOT1) / `dir[18]` (DOOM) | `PROVEN FROM CODE` |
| **Cluster Size** | `sectors_per_cluster = 8` × 512 = **4,096 bytes** | `PROVEN FROM CODE` [image_builder.c:165](file:///D:/Signatures_OS/tools/image_builder.c#L165) |

### WAV Header Parsing

- **File:** [audio_player.c:92–133](file:///D:/Signatures_OS/kernel/audio/session/audio_player.c#L92-L133)
- Reads `RiffHeader` (12 bytes), then iterates `ChunkHeader` entries searching for `"fmt "` and `"data"`.
- Validates `RIFF` + `WAVE` signatures.
- Rejects any format ≠ `{audio_format=1, num_channels=2, sample_rate=48000, bits_per_sample=16}`.
- Records `data_offset` (byte position after `"data"` chunk header) and `data_size`.

---

## 3. Stage 2: VFS Layer

| Property | Value | Validation |
|---|---|---|
| **File** | [vfs.c](file:///D:/Signatures_OS/kernel/vfs/vfs_legacy/src/vfs.c) | — |
| **FD Table** | `static VFS_FileDescriptor g_fd_table[MAX_OPEN_FILES]` (MAX=32) | `PROVEN FROM CODE` [vfs.c:161](file:///D:/Signatures_OS/kernel/vfs/vfs_legacy/src/vfs.c#L161) |
| **FD Range** | 3..31 (0–2 reserved for stdin/stdout/stderr) | `PROVEN FROM CODE` [vfs.c:168](file:///D:/Signatures_OS/kernel/vfs/vfs_legacy/src/vfs.c#L168) |
| **Per-FD State** | `{ bool in_use; VFS_Node* node; uint64_t offset; }` | `PROVEN FROM CODE` |
| **Thread Safety** | **NONE** — no locks on `g_fd_table[]` or `offset` | `PROVEN FROM CODE` |

### Key Functions

| Function | Location | Behavior |
|---|---|---|
| `vfs_open(path)` | [vfs.c:163](file:///D:/Signatures_OS/kernel/vfs/vfs_legacy/src/vfs.c#L163) | Allocates `VFS_Node` via `kmalloc`, calls `fat32_open()`, assigns FD slot. Each open gets **unique node + unique handle**. |
| `vfs_read(fd, buf, size)` | [vfs.c:204](file:///D:/Signatures_OS/kernel/vfs/vfs_legacy/src/vfs.c#L204) | Calls `node->fs_driver->read(node, offset, size, buffer)`, advances `g_fd_table[fd].offset += res`. |
| `vfs_seek(fd, offset, whence)` | [vfs.c:243](file:///D:/Signatures_OS/kernel/vfs/vfs_legacy/src/vfs.c#L243) | Supports SEEK_SET(0), SEEK_CUR(1), SEEK_END(2). Sets `g_fd_table[fd].offset`. |
| `vfs_close(fd)` | [vfs.c:261](file:///D:/Signatures_OS/kernel/vfs/vfs_legacy/src/vfs.c#L261) | Calls `fat32_close()`, then `kfree(node)`, clears FD slot. |

> [!IMPORTANT]
> Commit `1c0d2fb` upgraded VFS from a single mock FD (`fd=3` only, `mock_fd_node` global) to a proper 32-slot FD table with per-FD `VFS_Node` instances. This was necessary for multi-file support (DOOM + audio simultaneously).

---

## 4. Stage 3: FAT32 Driver — Cluster Walk & Disk Read

| Property | Value | Validation |
|---|---|---|
| **File** | [fat32.c](file:///D:/Signatures_OS/kernel/vfs/vfs_legacy/fs/fat32/src/fat32.c) | — |
| **File Handle Pool** | `static FAT32_FileHandle g_fat32_handles[MAX_FAT32_HANDLES]` | `PROVEN FROM CODE` |
| **Per-Handle Cache** | `cached_cluster_num`, `cached_cluster_index`, `cached_byte_offset` | `PROVEN FROM CODE` [fat32.c:182–184](file:///D:/Signatures_OS/kernel/vfs/vfs_legacy/fs/fat32/src/fat32.c#L182-L184) |
| **FAT Sector Cache** | `static uint8_t s_fat_sector_buf[512]` — single-sector LRU cache | `PROVEN FROM CODE` [fat32.c:248](file:///D:/Signatures_OS/kernel/vfs/vfs_legacy/fs/fat32/src/fat32.c#L248) |
| **Cluster Scratch** | `static uint8_t s_cluster_scratch[32768]` — **zero heap allocation** in hot path | `PROVEN FROM CODE` [fat32.c:600](file:///D:/Signatures_OS/kernel/vfs/vfs_legacy/fs/fat32/src/fat32.c#L600) |

### Read Path

1. `fat32_read(node, offset, size, buffer)` → [fat32.c:193](file:///D:/Signatures_OS/kernel/vfs/vfs_legacy/fs/fat32/src/fat32.c#L193)
2. Extracts `FAT32_FileHandle*` from `node->private_data`.
3. Calls `fat32_read_file(vol, start_cluster, file_size, buffer, offset, handle)` → [fat32.c:626](file:///D:/Signatures_OS/kernel/vfs/vfs_legacy/fs/fat32/src/fat32.c#L626)
4. **Sequential Cluster Cursor Cache** check: if `offset >= handle->cached_byte_offset`, starts walk from cached cluster (O(1) for sequential reads). Otherwise resets to `start_cluster` (O(N) from beginning).
5. Calls `fat32_walk_cluster_chain(vol, walk_start_cluster, callback, &ctx)` → [fat32.c:280](file:///D:/Signatures_OS/kernel/vfs/vfs_legacy/fs/fat32/src/fat32.c#L280)
6. Each cluster: `fat32_read_file_callback()` → reads cluster via `block_device_read()` into `s_cluster_scratch[32768]`, copies relevant bytes into output buffer.
7. After walk, updates cache: `handle->cached_cluster_num = ctx.last_accessed_cluster`.

### `fat32_next_cluster(vol, cluster)` — [fat32.c:252](file:///D:/Signatures_OS/kernel/vfs/vfs_legacy/fs/fat32/src/fat32.c#L252)

- Reads FAT entry for `cluster`.
- Uses `s_fat_sector_buf[512]` single-sector cache to avoid re-reading the same FAT sector on sequential cluster walks.
- Returns next cluster number or `0x0FFFFFF8+` for EOF.

> [!NOTE]
> The cluster cache was added in commit `c0133b9` to fix O(N²) sequential read degradation. Commit `1c0d2fb` migrated it from global state to per-handle state.

---

## 5. Stage 4: ATA PIO Driver — Sector Transfer

| Property | Value | Validation |
|---|---|---|
| **File** | [ata.c](file:///D:/Signatures_OS/kernel/drivers/storage_legacy/storage/src/ata.c) | — |
| **Transfer Mode** | PIO (Programmed I/O) via `rep insw` hardware string instruction | `PROVEN FROM CODE` [ata.c:35–37](file:///D:/Signatures_OS/kernel/drivers/storage_legacy/storage/src/ata.c#L35-L37) |
| **Lock** | `static volatile bool ata_lock` — spinlock via `__sync_lock_test_and_set` | `PROVEN FROM CODE` |
| **Wait Functions** | `ata_wait_bsy()` and `ata_wait_drq()` — timeout after 1,000,000 iterations, return `false` on ERR/DF | `PROVEN FROM CODE` |
| **IRQ Used** | **NONE** — pure polling PIO | `PROVEN FROM CODE` |
| **Interrupt Masking** | `pushfq; popq; cli` before `rep insw`, restore IF after | `PROVEN FROM CODE` [ata.c:73–76](file:///D:/Signatures_OS/kernel/drivers/storage_legacy/storage/src/ata.c#L73-L76) |

### Critical Behavior

```c
// Per-sector transfer (512 bytes = 256 words):
uint64_t flags;
__asm__ volatile("pushfq; popq %0; cli" : "=r"(flags));
ata_insw(io_base + ATA_REG_DATA, ptr, 256);   // 1 VM-Exit per sector (was 256 before fix)
if (flags & 0x200) __asm__ volatile("sti");
ptr += 512;
```

**Key insight:** During each `rep insw`, interrupts are **disabled** (`cli`). For a 16 KB read (32 sectors), interrupts are disabled for 32 consecutive sector transfers. At ~18–32 ms per 16 KB read (post-`rep insw` fix), this means the **IRQ 0 timer tick is blocked** for the duration of each ATA sector transfer burst.

> [!WARNING]
> The `ata_lock` spinlock provides reentrancy protection but **no preemption fairness**. If the `AudioSvc` task enters `vfs_read()` and calls `ata_read_sectors_internal()`, it holds the lock and disables interrupts for each sector, blocking the 1000 Hz timer tick that drives `audio_realtime_worker_pump()`.

---

## 6. Stage 5: Audio Player (Producer)

| Property | Value | Validation |
|---|---|---|
| **File** | [audio_player.c](file:///D:/Signatures_OS/kernel/audio/session/audio_player.c) | — |
| **Global Session** | `static AudioSession g_audio_session` | `PROVEN FROM CODE` [audio_player.c:44](file:///D:/Signatures_OS/kernel/audio/session/audio_player.c#L44) |
| **Read Buffer** | `static uint8_t g_read_buffer[32768]` (32 KB) | `PROVEN FROM CODE` [audio_player.c:53](file:///D:/Signatures_OS/kernel/audio/session/audio_player.c#L53) |
| **Chunk Size** | `PRODUCER_CHUNK_SIZE = 16384` (16 KB) | `PROVEN FROM CODE` [audio_player.c:241](file:///D:/Signatures_OS/kernel/audio/session/audio_player.c#L241) |
| **States** | `STOPPED → OPENED → PLAYING → PAUSED → STOPPED` | `PROVEN FROM CODE` |

### `audio_player_update()` — [audio_player.c:247](file:///D:/Signatures_OS/kernel/audio/session/audio_player.c#L247)

This is the **producer pump**, called from the `AudioSvc` background task every 20 ms.

**Watermark Constants:**

| Constant | Value | Purpose |
|---|---|---|
| `PRODUCER_HIGH_WATERMARK_PCT` | 80% | Start refilling when occupancy drops below this |
| `PRODUCER_LOW_WATERMARK_PCT` | 40% | Aggressive refill (4 chunks) |
| `PRODUCER_CRITICAL_WATERMARK_PCT` | 20% | Emergency refill (8 chunks) |
| `PRODUCER_REFILL_TARGET_PCT` | 90% | Stop refilling when above this |
| `PRODUCER_CHUNK_SIZE` | 16,384 bytes | Single VFS read unit |

**Algorithm:**

1. Check if `bytes_played >= data_size` → loop (seek back to `data_offset`).
2. Compute `occupancy_pct = (available_bytes * 100) / capacity_bytes`.
3. If `occupancy_pct < 80%` OR state is `OPENED` (initial prefill):
   - Determine `max_chunks` based on occupancy (2/4/8/16).
   - Loop up to `max_chunks` times:
     - Check free space ≥ `CHUNK_SIZE + 4`.
     - Check `occupancy_pct < REFILL_TARGET_PCT` (skip if ≥ 90%, unless initial prefill).
     - `vfs_read(fd, g_read_buffer, to_read)` → **BLOCKING I/O**.
     - Construct `AudioPcmPacket`, call `audio_stream_write(stream_id, &packet)`.
     - Increment `bytes_played`.

> [!CAUTION]
> **Critical starvation vector:** When `max_chunks = 8` (occupancy < 20%), the producer executes up to 8 consecutive blocking `vfs_read()` calls without yielding. Each 16 KB read takes 18–32 ms (post-fix). Total blocking time: **144–256 ms**. During this entire period, the `AudioSvc` task does not yield, and if this task is holding the CPU, the scheduler cannot run other tasks.

### `audio_player_play()` — [audio_player.c:178](file:///D:/Signatures_OS/kernel/audio/session/audio_player.c#L178)

**Synchronous prefill loop before DMA start:**

```c
while (capacity - available >= 16384 + 4 && bytes_played < data_size) {
    audio_player_update();  // Fills ring buffer with up to 16 chunks (256 KB)
}
```

This runs **before** `sti` in ATM mode, or before the first desktop frame in normal boot. It pre-fills the ring buffer to ~90%+ before starting DMA.

---

## 7. Stage 6: Ring Buffer (256 KB)

| Property | Value | Validation |
|---|---|---|
| **File** | [audio_buffer.c](file:///D:/Signatures_OS/kernel/audio/streams/audio_buffer.c) | — |
| **Stream File** | [audio_stream.c](file:///D:/Signatures_OS/kernel/audio/streams/audio_stream.c) | — |
| **Capacity** | `DEFAULT_RING_BUFFER_SIZE = 262144` (256 KB) | `PROVEN FROM CODE` [audio_stream.c:5](file:///D:/Signatures_OS/kernel/audio/streams/audio_stream.c#L5) |
| **Type** | Classic circular byte buffer with `head` (write) and `tail` (read) pointers | `PROVEN FROM CODE` |
| **Full Detection** | 1 byte reserved: `free_space = capacity - available - 1` | `PROVEN FROM CODE` [audio_buffer.c:62](file:///D:/Signatures_OS/kernel/audio/streams/audio_buffer.c#L62) |
| **Usable Capacity** | 262,143 bytes | Derived |
| **Audio Duration** | 262,143 / 192,000 = **~1.365 seconds** of audio | Derived |
| **Allocation** | `kmalloc(262144)` during `audio_stream_create_obj()` | `PROVEN FROM CODE` |
| **Thread Safety** | **NONE** — no locks, no atomics, no memory barriers | `PROVEN FROM CODE` |

### Data Flow Through Ring Buffer

| Operation | Function | Who Calls | Context |
|---|---|---|---|
| **Write** | `audio_buffer_write(buf, data, size)` | `audio_stream_write()` ← `audio_player_update()` | `AudioSvc` task (producer) |
| **Read** | `audio_buffer_read(buf, data, size)` | `audio_stream_read()` ← `audio_mixer_process()` | IRQ 0 context (consumer) |

> [!WARNING]
> **Race condition:** The producer (`AudioSvc` task) writes to `head` while the consumer (IRQ 0 handler) reads from `tail`. There are **no memory barriers** or atomic operations protecting these accesses. On x86, TSO (Total Store Ordering) provides some implicit guarantees, but this is still formally a data race.

### API Layer — [audio_api.c](file:///D:/Signatures_OS/kernel/audio/api/audio_api.c)

| Function | Line | Behavior |
|---|---|---|
| `audio_stream_write(id, packet)` | [84](file:///D:/Signatures_OS/kernel/audio/api/audio_api.c#L84) | Validates format match, calls `audio_buffer_write()` |
| `audio_stream_read(id, buf, size)` | [106](file:///D:/Signatures_OS/kernel/audio/api/audio_api.c#L106) | Calls `audio_buffer_read()`, updates stats |
| `audio_stream_available(id)` | [138](file:///D:/Signatures_OS/kernel/audio/api/audio_api.c#L138) | Returns `audio_buffer_available()` |
| `audio_stream_capacity(id)` | [145](file:///D:/Signatures_OS/kernel/audio/api/audio_api.c#L145) | Returns `ring_buffer->capacity` (262144) |

---

## 8. Stage 7: Audio Mixer

| Property | Value | Validation |
|---|---|---|
| **File** | [audio_mixer.c](file:///D:/Signatures_OS/kernel/audio/mixer/audio_mixer.c) | — |
| **Mix Buffer** | `MIX_BUFFER_FRAMES = 1024` → max 1024 frames per call | `PROVEN FROM CODE` |
| **Accumulator** | `static int32_t accum_buffer[1024 * 2]` (8 KB, stereo) | `PROVEN FROM CODE` [audio_mixer.c:98](file:///D:/Signatures_OS/kernel/audio/mixer/audio_mixer.c#L98) |
| **Temp Buffer** | `static uint8_t temp_buffer[1024 * 4]` (4 KB) | `PROVEN FROM CODE` [audio_mixer.c:99](file:///D:/Signatures_OS/kernel/audio/mixer/audio_mixer.c#L99) |
| **Max Streams** | `MAX_MIXER_STREAMS = 16` | `PROVEN FROM CODE` |

### `audio_mixer_process(output_buffer, max_bytes, target_format)` — [audio_mixer.c:83](file:///D:/Signatures_OS/kernel/audio/mixer/audio_mixer.c#L83)

**Called from:** `ac97_playback_update()` inside IRQ 0 context.

**Algorithm:**

1. Compute `frames_to_mix = max_bytes / bytes_per_frame` (capped at 1024).
2. Compute `mix_bytes = frames_to_mix × bpf`.
3. Clear `accum_buffer[]` to zero.
4. For each registered stream:
   - Check `audio_stream_available(stream_id) >= mix_bytes`.
   - **If available:** Read `mix_bytes` from ring buffer, apply volume, accumulate into `accum_buffer`.
   - **If NOT available:** Increment `stream->stats.underrun_counter`. **No data mixed for this stream.**
5. Normalize `accum_buffer` → clamp to 16-bit → write to `output_buffer`.
6. **If `active_mixed == 0`** (no streams had enough data): `g_mixer_silence_bytes += mix_bytes`. Output buffer contains **normalized zeros = silence**.

> [!IMPORTANT]
> **Starvation behavior:** When the ring buffer has fewer bytes than `mix_bytes` (typically 4096 bytes = 1024 frames × 4 bytes/frame), the mixer outputs **pure silence** (zeroed samples normalized to 16-bit). This is the direct mechanism that produces the "shhhhh" static/noise heard during the regression — the DMA plays silence/zero-fill data pushed by the mixer.

---

## 9. Stage 8: AC97 DMA Buffer & BDL

| Property | Value | Validation |
|---|---|---|
| **File** | [ac97_dma.c](file:///D:/Signatures_OS/kernel/audio/drivers/ac97/ac97_dma.c) | — |
| **PCM Buffer Size** | `131,072 bytes` (128 KB) | `PROVEN FROM CODE` [ac97_playback.c:69](file:///D:/Signatures_OS/kernel/audio/drivers/ac97/ac97_playback.c#L69) |
| **BDL Entries** | `AC97_BDL_ENTRIES = 32` | `PROVEN FROM CODE` |
| **Bytes Per Descriptor** | 131,072 / 32 = **4,096 bytes** | Derived |
| **Samples Per Descriptor** | 4,096 / 2 = 2,048 samples (length field is in sample count) | `PROVEN FROM CODE` [ac97_dma.c:91](file:///D:/Signatures_OS/kernel/audio/drivers/ac97/ac97_dma.c#L91) |
| **Audio Per Descriptor** | 4,096 / 192,000 = **~21.33 ms** | Derived |
| **Total DMA Audio** | 128 KB / 192,000 = **~682 ms** | Derived |
| **IOC Flags** | `flags = 0` — No interrupt-on-completion (polling mode) | `PROVEN FROM CODE` [ac97_dma.c:92](file:///D:/Signatures_OS/kernel/audio/drivers/ac97/ac97_dma.c#L92) |
| **Allocation** | `kmalloc(131072)` for PCM buffer, `kmalloc(32 × sizeof(Ac97BdlEntry))` for BDL | `PROVEN FROM CODE` |
| **Physical Mapping** | `get_phys()` via `vmm_get_physical_address()`, assumes **contiguous physical pages** | `PROVEN FROM CODE` [ac97_dma.c:90](file:///D:/Signatures_OS/kernel/audio/drivers/ac97/ac97_dma.c#L90) |

### BDL Entry Structure

```c
typedef struct {
    uint32_t buffer_phys_addr;   // Physical address of PCM data for this descriptor
    uint16_t length;              // Length in SAMPLES (not bytes) — each sample = 16-bit
    uint16_t flags;               // Bit 15: IOC, Bit 14: BUP — both 0 (polling mode)
} __attribute__((packed)) Ac97BdlEntry;
```

### Physical Contiguity Audit

[ac97_dma.c:97–118](file:///D:/Signatures_OS/kernel/audio/drivers/ac97/ac97_dma.c#L97-L118) — At prepare time, the driver iterates all 32 descriptors and compares `assumed_phys` (linear offset from `pcm_buffer_phys`) against `real_phys` (per-page VMM translation). Logs `MISMATCH` or `ALL 32 DESCRIPTORS MATCH`.

> [!WARNING]
> **Fragile assumption:** `kmalloc(131072)` must return physically contiguous memory. If the heap allocator returns virtually contiguous but physically fragmented pages, descriptors will point to **wrong physical addresses** and the DMA controller will read garbage → noise/static. The contiguity check exists as a diagnostic but does **not abort** on failure.

---

## 10. Stage 9: AC97 Playback Update (Consumer/IRQ)

| Property | Value | Validation |
|---|---|---|
| **File** | [ac97_playback.c](file:///D:/Signatures_OS/kernel/audio/drivers/ac97/ac97_playback.c) | — |
| **Trigger** | IRQ 0 @ 1000 Hz → `timer_tick_handler()` → `audio_realtime_worker_pump()` → `audio_hal_update_pointers(0)` → `ac97_hal_update_pointers()` → `ac97_playback_update()` | `PROVEN FROM CODE` |
| **State Guard** | Returns immediately if `g_pb_state != AC97_PB_STATE_RUNNING` | `PROVEN FROM CODE` |

### `ac97_playback_update()` — [ac97_playback.c:165](file:///D:/Signatures_OS/kernel/audio/drivers/ac97/ac97_playback.c#L165)

**Algorithm:**

1. Read hardware registers: `CIV`, `SR`, `CR`.
2. Clear write-1-to-clear status bits (`BCIS`, `LVBCI`, `FIFO ERR`).
3. **DCH Recovery:** If `SR & DCH` (DMA halted):
   - Refill ALL 32 descriptors from mixer.
   - Restart DMA (`CR |= 0x01`).
   - Re-read actual hardware `CIV` to synchronize software state.
   - Set `LVI = (CIV + 31) % 32`.
   - Return.
4. **No Rotation:** If `CIV == g_last_civ`, return (hardware hasn't advanced).
5. **Refill consumed descriptors:**
   - Walk from `g_last_civ` to `CIV`, calling `audio_mixer_process()` for each consumed descriptor.
   - Each descriptor: 4,096 bytes of PCM data.
6. Update `g_last_civ = CIV`.
7. Set `LVI = (CIV + 31) % 32` → gives hardware 31 descriptors of runway.

### State Variables

| Variable | Type | Purpose |
|---|---|---|
| `g_last_civ` | `uint8_t` | Software shadow of last-seen hardware CIV |
| `g_lvi` | `uint8_t` | Software-managed LVI value |
| `g_dma_mgr` | `Ac97DmaManager*` | Pointer to DMA buffer/BDL manager |
| `g_nabm_base` | `uint16_t` | NABM BAR I/O port base |
| `g_pb_state` | `Ac97PlaybackState` | State machine (UNINITIALIZED/PREPARED/STARTING/RUNNING/STOPPING/STOPPED/ERROR) |
| `g_pb_telemetry` | `Ac97PlaybackTelemetry` | Accumulated counters: bytes_sent, frames_played, underruns, restarts |

---

## 11. Stage 10: Hardware Registers & Physical Device

| Register | Offset from NABM | Width | Purpose |
|---|---|---|---|
| **BDBAR** | `0x10` | 32-bit | Buffer Descriptor Base Address Register — physical address of BDL |
| **CIV** | `0x14` | 8-bit | Current Index Value — descriptor currently being played by hardware |
| **LVI** | `0x15` | 8-bit | Last Valid Index — last descriptor hardware is allowed to play before halting |
| **SR** | `0x16` | 16-bit | Status Register — DCH (bit 1), BCIS (bit 3), LVBCI (bit 2), FIFO ERR (bit 4) |
| **PICB** | `0x18` | 16-bit | Position In Current Buffer — samples remaining in current descriptor |
| **CR** | `0x1B` | 8-bit | Control Register — RPBM (bit 0: run/pause), RR (bit 1: reset) |

### LVI Strategy

`LVI = (CIV + 31) % 32` — always 1 behind CIV in the circular ring. This gives the hardware the maximum 31-descriptor runway. Hardware plays from CIV through LVI, then halts (sets DCH). Software must update LVI before hardware catches up.

### Hardware Consumption Rate

At 48 kHz stereo 16-bit: 192,000 bytes/sec.  
Each descriptor: 4,096 bytes → 21.33 ms per descriptor.  
32 descriptors: 682 ms total DMA buffer.  
Hardware advances CIV approximately every 21.33 ms.

---

## 12. Initialization Order

### Normal Boot Path (GUI Desktop)

| Step | Function | File | Line |
|---|---|---|---|
| 1 | `audio_init()` | [kernel.c:577](file:///D:/Signatures_OS/kernel/kernel.c#L577) | Core audio subsystem |
| 2 | `audio_mixer_init()` | [kernel.c:578](file:///D:/Signatures_OS/kernel/kernel.c#L578) | Mixer stream registry |
| 3 | `audio_hal_init()` | [kernel.c:579](file:///D:/Signatures_OS/kernel/kernel.c#L579) | PCI scan → AC97 driver register → codec init → DMA init → playback init |
| 4 | `scheduler_create_kernel_task("AudioSvc", audio_service_entry)` | [kernel.c:581](file:///D:/Signatures_OS/kernel/kernel.c#L581) | Background producer task |
| 5 | `Desktop_Shell_Initialize()` | [kernel.c:688](file:///D:/Signatures_OS/kernel/kernel.c#L688) | Shell setup |
| 6 | `scheduler_register_boot_task()` | [kernel.c:720](file:///D:/Signatures_OS/kernel/kernel.c#L720) | Boot task becomes schedulable |
| 7 | `__asm__ volatile("sti")` | [kernel.c:738](file:///D:/Signatures_OS/kernel/kernel.c#L738) | **Interrupts enabled** — IRQ 0 starts firing |
| 8 | First desktop frame → `audio_player_open("/BOOT1.WAV")` + `audio_player_play()` | [desktop_shell.c:598–599](file:///D:/Signatures_OS/kernel/shell/desktop_shell/desktop_shell.c#L598-L599) | Deferred to first frame |

### Audio HAL Init Chain

```
audio_hal_init()
  → audio_driver_registry_init()
  → ac97_driver_register()                  // registers ac97_driver struct
  → audio_driver_registry_discover_active()
      → PCI bus scan (bus 0..255, slot 0..31)
      → Match class 0x04, subclass 0x01 or 0x03
      → ac97_hal_init(pci_info)
          → Read BAR0 (NAM), BAR1 (NABM)
          → Enable PCI bus master + I/O space
          → ac97_codec_init_base(nam_bar, nabm_bar)
          → ac97_codec_verify_and_configure()
          → ac97_dma_init(nabm_bar)
          → ac97_playback_init()
```

### Audio Test Mode (ATM) Path

| Step | Function | File |
|---|---|---|
| 1 | `audio_test_mode_entry()` | [audio_test_mode.c:35](file:///D:/Signatures_OS/kernel/audio/diagnostics/audio_test_mode.c#L35) |
| 2 | `audio_init()` + `audio_mixer_init()` + `audio_hal_init()` | Same as above |
| 3 | `audio_player_open("/DEMO1.WAV")` | Synchronous prefill |
| 4 | `audio_player_play()` | Prefill + DMA start |
| 5 | `scheduler_create_kernel_task("AudioSvc", audio_service_entry)` | [kernel.c:550](file:///D:/Signatures_OS/kernel/kernel.c#L550) |
| 6 | `scheduler_register_boot_task()` + `sti` | [kernel.c:554–560](file:///D:/Signatures_OS/kernel/kernel.c#L554-L560) |
| 7 | Boot task idle loop: `while(1) { scheduler_yield(); }` | [kernel.c:567–570](file:///D:/Signatures_OS/kernel/kernel.c#L567-L570) |

---

## 13. Scheduler & Task Model

| Property | Value | Validation |
|---|---|---|
| **File** | [scheduler.c](file:///D:/Signatures_OS/kernel/core/scheduler/src/scheduler.c) | — |
| **Tick Rate** | 1000 Hz (PIT IRQ 0) | `PROVEN FROM CODE` |
| **AudioSvc Sleep** | `scheduler_sleep(20)` → 20 ms between `audio_player_update()` calls | `PROVEN FROM CODE` [kernel.c:312](file:///D:/Signatures_OS/kernel/kernel.c#L312) |
| **Sleep Mechanism** | Sets `task->wake_tick = ticks + 20`, moves to `sleep_queue`, busy-waits on `hlt` | `PROVEN FROM CODE` [scheduler.c:258–274](file:///D:/Signatures_OS/kernel/core/scheduler/src/scheduler.c#L258-L274) |
| **Yield Ping-Pong Guard** | If `last_run_tick == scheduler_tick_count`, executes `sti; hlt; return` (waits for next tick) | `PROVEN FROM CODE` (working tree diff) |

### `scheduler_on_tick()` — [scheduler.c:296](file:///D:/Signatures_OS/kernel/core/scheduler/src/scheduler.c#L296)

**Execution order within IRQ 0:**

1. `timer_tick_handler()` increments `system_ticks`.
2. `audio_realtime_worker_pump()` — **runs BEFORE scheduler**.
3. `context_save_state(current_task)`.
4. `scheduler_on_tick()`:
   - Wake sleeping tasks whose `wake_tick <= current_time`.
   - Decrement `current_task->quantum`.
   - If quantum expired or task sleeping: context switch.
5. `context_restore_state(new_task)`.

> [!IMPORTANT]
> The audio realtime pump runs **inside the IRQ 0 handler**, before the scheduler. This guarantees it executes at 1000 Hz regardless of task scheduling. However, if `AudioSvc` is inside `ata_read_sectors_internal()` with interrupts disabled (`cli`), the IRQ 0 handler is **deferred** until `sti` re-enables interrupts. This causes the pump to miss ticks.

---

## 14. Ring Buffer Watermark Strategy

### "Staged Proactive Watermark Refill" — [audio_player.c:265–331](file:///D:/Signatures_OS/kernel/audio/session/audio_player.c#L265-L331)

```
Ring Buffer (256 KB)
├── 0%   ─── EMPTY (mixer outputs silence)
├── 20%  ─── CRITICAL_WATERMARK → max_chunks = 8 (128 KB read burst)
├── 40%  ─── LOW_WATERMARK      → max_chunks = 4 (64 KB read burst)
├── 80%  ─── HIGH_WATERMARK     → max_chunks = 2 (32 KB read burst)
├── 90%  ─── REFILL_TARGET      → stop refilling
└── 100% ─── FULL (writes silently truncated by ring buffer)
```

### Producer Worker — [audio_producer_worker.c](file:///D:/Signatures_OS/kernel/audio/session/audio_producer_worker.c)

Additional watermark state machine wrapping `audio_player_update()`:
- `PRODUCER_WATERMARK_LOW_PCT = 50%` — begin refilling.
- `PRODUCER_WATERMARK_HIGH_PCT = 80%` — stop refilling.
- `PRODUCER_WATERMARK_TARGET_PCT = 70%` — refill if below target.

> [!NOTE]
> In the current code, the `AudioSvc` task calls `audio_player_update()` directly (not via `audio_producer_worker_run()`). The producer worker is only used in the diagnostic mode path. The `AudioSvc` entry function at [kernel.c:304–313](file:///D:/Signatures_OS/kernel/kernel.c#L304-L313) calls `audio_player_update()` then `scheduler_sleep(20)`.

---

## 15. Regression Analysis: Current vs. Last-Known-Good

### Last Known-Good State

Commit `1c0d2fb` ("fix(audio): eliminate ~300ms rhythmic stutter via rep insw ATA hardware batch transfers") was the **last verified working audio** state. The existing [AUDIO_ENGINEERING_ROOT_CAUSE_AUDIT.md](file:///D:/Signatures_OS/docs/AUDIO_ENGINEERING_ROOT_CAUSE_AUDIT.md) documents verified telemetry:

```
STREAM: occ=81%-85%   ← Ring buffer healthy
MIXER: silence=0      ← Zero silence bytes
read_time=18-32ms     ← Fast disk reads (rep insw)
```

### Commits After Last-Known-Good

| Commit | Description | Audio-Relevant Changes |
|---|---|---|
| `b5d41a0` | Fix GUI Presentation Engine and ATA Stuttering | Modifies `ata.c` (simplified), `bwe_compositor.c`, `surface.c` — **ATA changes may affect read performance** |
| `1e07300` | Fix presentation pipeline horizontal repetition | `bwe_compositor.c`, `graphics.c` — **no audio changes** |
| `b96ad50` | DOOM Rendering Pipeline Complete | `audio_player.c` (vfs_seek signature fix), `kernel.c` (massive expansion), `desktop_shell.c` (boot audio), `vfs.c` (vfs_seek 3-arg), `fat32.c` (+1 line), `heap.c` (major rewrite 389 lines changed), `scheduler` (not in diff but in working tree) |

### Critical Changes in `b96ad50`

#### 1. `audio_player.c` — vfs_seek Signature Change
```diff
-  extern int vfs_seek(int fd, uint64_t offset);
-  vfs_seek(g_audio_session.fd, g_audio_session.data_offset);
+  extern int vfs_seek(int fd, uint64_t offset, int whence);
+  vfs_seek(g_audio_session.fd, g_audio_session.data_offset, 0);
```
**Impact:** Cosmetic fix. The 3-arg version existed in vfs.c; the 2-arg `extern` declaration was wrong. **Not a regression cause.**

#### 2. `heap.c` — Major Rewrite (389 lines changed)
The heap allocator was substantially rewritten. If the new allocator returns memory that is **not physically contiguous** for the 128 KB DMA buffer allocation (`kmalloc(131072)` in `ac97_dma_prepare()`), then BDL descriptors will point to wrong physical pages → **DMA reads garbage → noise/static**.

#### 3. `kernel.c` — DOOM Process Loading
DOOM.ELF loading via `elf_load_image()` consumes significant heap memory before audio initialization. This may fragment the heap, preventing the 128 KB contiguous allocation needed for AC97 DMA.

#### 4. `vfs.c` — vfs_seek Now 3-Argument
```c
int vfs_seek(int fd, uint64_t offset, int whence)
```
**Impact:** Correct signature. All callers updated. **Not a regression cause.**

#### 5. Working Tree Change: `scheduler_yield()` Ping-Pong Guard
```diff
-  return; // Reject immediate re-yield in the same tick
+  __asm__ volatile("sti");
+  __asm__ volatile("hlt" : : : "memory");
+  return;
```
**Impact:** Previously, rapid yield loops would spin the CPU. Now they `hlt` until next interrupt. This could **delay** `AudioSvc` wakeup by up to 1 ms (next timer tick). Minor but could contribute to cumulative latency.

---

## 16. Root Cause Hypothesis for Current Regression

### Primary Suspect: Heap Fragmentation Breaking DMA Buffer Physical Contiguity

**Evidence chain:**

1. Audio plays correctly for ~2–3 seconds → initial prefill succeeded, DMA buffer was initially populated with correct PCM data.
2. After 2–3 seconds → noise/static begins → the DMA is playing data, but it's **not valid PCM**.
3. The regression correlates with `b96ad50` which:
   - Added DOOM.ELF loading (consumes large heap blocks before audio init in some paths).
   - Rewrote `heap.c` (389 lines changed).
4. `ac97_dma_prepare()` allocates 128 KB via `kmalloc(131072)` and computes BDL physical addresses assuming `pcm_buffer_phys + (i * 4096)` is contiguous.
5. If the new heap returns virtual pages backed by **non-contiguous physical frames**, the BDL entries point to random physical memory.
6. The DMA controller reads from those physical addresses → garbage data → noise.
7. The initial ~2–3 seconds work because the initial prefill fills all 32 descriptors correctly from the ring buffer, and the DMA plays through them. But when `ac97_playback_update()` refills consumed descriptors, it writes to `g_dma_mgr->pcm_buffer + (idx * chunk_bytes)` (virtual address, correct), but the hardware reads from `bdl[idx].buffer_phys_addr` (potentially wrong physical address).

### Secondary Suspect: Scheduler Starvation of AudioSvc

**Evidence chain:**

1. DOOM runs as a Ring 3 process consuming CPU for rendering.
2. The boot task main loop runs horse_engine, compositor, damage tracking, and desktop shell updates.
3. `AudioSvc` sleeps for 20 ms, wakes, calls `audio_player_update()`, sleeps again.
4. If scheduler quantum or priority allows DOOM/Boot task to starve `AudioSvc`:
   - Ring buffer drains below mixer threshold.
   - Mixer outputs silence.
   - DMA plays silence → "shhhhh".
5. The modified `scheduler_yield()` with `hlt` could add up to 1 ms latency per yield cycle.

### Tertiary Suspect: ATA Changes in `b5d41a0`

Commit `b5d41a0` "Fix GUI Presentation Engine and ATA Stuttering" modified `ata.c` with "40 ++++-----------" (net reduction). If this reverted or altered the `rep insw` optimization, disk read latency would return to the pre-fix ~82 ms/chunk level, causing producer starvation.

---

## 17. Complete Call Graph (End-to-End)

### Producer Path (AudioSvc Task, every 20 ms)

```
audio_service_entry()                           [kernel.c:304]
  └─► audio_player_update()                     [audio_player.c:247]
        ├─► audio_stream_available(stream_id)   [audio_api.c:138]
        │     └─► audio_buffer_available(ring)  [audio_buffer.c:49]
        ├─► audio_stream_capacity(stream_id)    [audio_api.c:145]
        ├─► vfs_read(fd, g_read_buffer, 16384)  [vfs.c:204]
        │     └─► fat32_read(node, offset, sz)  [fat32.c:193]
        │           └─► fat32_read_file(vol, ...)  [fat32.c:626]
        │                 └─► fat32_walk_cluster_chain()  [fat32.c:280]
        │                       └─► fat32_read_file_callback()  [fat32.c:583]
        │                             └─► block_device_read()
        │                                   └─► ata_read_sectors_internal()  [ata.c]
        │                                         ├─► ata_wait_bsy()
        │                                         ├─► ata_wait_drq()
        │                                         └─► ata_insw() (cli; rep insw; sti)
        └─► audio_stream_write(stream_id, &pkt) [audio_api.c:84]
              └─► audio_buffer_write(ring, data) [audio_buffer.c:75]
```

### Consumer Path (IRQ 0, every 1 ms)

```
timer_tick_handler(regs)                        [timer.c:13]
  ├─► system_ticks++
  ├─► audio_realtime_worker_pump()              [audio_realtime_worker.c:35]
  │     └─► audio_hal_update_pointers(0)        [audio_hal.c:66]
  │           └─► ac97_hal_update_pointers()    [ac97.c:66]
  │                 └─► ac97_playback_update()  [ac97_playback.c:165]
  │                       ├─► io_in8(CIV)
  │                       ├─► io_in16(SR)
  │                       ├─► [if CIV rotated] audio_mixer_process()  [audio_mixer.c:83]
  │                       │     ├─► audio_stream_read(stream_id)      [audio_api.c:106]
  │                       │     │     └─► audio_buffer_read(ring)     [audio_buffer.c:105]
  │                       │     ├─► audio_volume_apply_16()
  │                       │     ├─► audio_math_mix_16()               [audio_mix_math.c:21]
  │                       │     └─► audio_math_normalize_16()         [audio_mix_math.c:39]
  │                       └─► io_out8(LVI)
  ├─► context_save_state()
  ├─► scheduler_on_tick()                       [scheduler.c:296]
  └─► context_restore_state()
```

### Boot Audio Path (Normal Desktop Boot)

```
kernel_main()
  ├─► audio_init() + audio_mixer_init() + audio_hal_init()
  ├─► scheduler_create_kernel_task("AudioSvc")
  ├─► Desktop_Shell_Initialize()
  ├─► sti
  └─► main_loop → desktop_shell_render()
        └─► [first frame] audio_player_open("/BOOT1.WAV")
              └─► audio_player_play()
                    ├─► [sync prefill loop] audio_player_update() × N
                    ├─► audio_hal_start_stream(48000, 2, 16)
                    │     └─► ac97_playback_prepare(131072)
                    │     └─► ac97_playback_start()
                    │           ├─► [fill all 32 desc from mixer]
                    │           ├─► LVI = 31
                    │           └─► CR |= 0x01  (RPBM — start DMA)
                    └─► audio_realtime_worker_start()
```

---

> [!CAUTION]
> ## Summary of Risk Vectors (Ranked by Likelihood)
>
> 1. **Heap `kmalloc(131072)` returns non-contiguous physical pages** after `heap.c` rewrite → BDL physical addresses wrong → DMA reads garbage → noise. **HIGHEST RISK.**
>
> 2. **Scheduler starvation** of `AudioSvc` due to DOOM + compositor CPU load → ring buffer drains → mixer outputs silence → DMA plays zeros → "shhhhh". **HIGH RISK.**
>
> 3. **ATA `rep insw` optimization reverted** in commit `b5d41a0` → disk read latency returns to ~82 ms → producer can't keep up → ring buffer starvation. **MEDIUM RISK.**
>
> 4. **Ring buffer SPSC race condition** between `AudioSvc` (writer) and IRQ 0 (reader) without memory barriers → corrupted head/tail → mixer reads stale/zero data. **LOW RISK** (x86 TSO mitigates most cases).
>
> 5. **`s_cluster_scratch[32768]` static buffer** shared across all FAT32 reads — if DOOM file I/O runs concurrently with audio file I/O, the scratch buffer gets clobbered → corrupted PCM data written to ring buffer. **MEDIUM RISK.**

---

*End of Audit. NO CODE WAS MODIFIED.*
