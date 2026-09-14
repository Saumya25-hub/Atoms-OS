# ATOMS OS — TWO-CHANNEL MEDIA DEBUGGER ARCHITECTURE
**Document ID**: `ARCH-DEBUG-2026-09-13-V1`  
**Classification**: SYSTEM DESIGN & INTEGRATION SPECIFICATION  
**Target Architecture**: Intel Haswell H81 Motherboard (Native UEFI Mode, Core i3 4th Gen, 8GB RAM)  
**Status**: ARCHITECTURAL DESIGN (PHASE 2 READY) — READ-ONLY PROTOCOL COMPLIANT

---

## 1. OBJECTIVE & CORE PHILOSOPHY

The goal of the ATOMS Media Debugger is to eliminate blind guessing, external monitor photography, and post-mortem assumptions during media playback bring-up.

When an external host or human developer asks:
> *"Why is the screen black or frozen?"*

The system must automatically answer with zero ambiguity:
- **WHAT**: Exact bitstream error, NAL unit, or macroblock failure.
- **WHERE**: Exact pipeline stage, source file, line number, and function.
- **WHEN**: Monotonic timestamp and media PTS.
- **WHICH PROCESS & RING**: PID and execution privilege (Ring 0 vs. Ring 3).
- **WHICH BUFFER & SURFACE**: Memory virtual/physical address, stride, format, dimensions.
- **FIRST FAILURE**: The locked root-cause event that triggered the cascade.

---

## 2. REUSE OF EXISTING ATOMS DEBUG INFRASTRUCTURE

ATOMS already possesses robust, bare-metal debug facilities. **Zero parallel debug architectures will be created.** The Media Debugger directly integrates with:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                      EXISTING ATOMS DEBUG INFRASTRUCTURE                    │
├──────────────────────────┬──────────────────────────┬───────────────────────┤
│ Facility                 │ Hardware / Transport     │ Implementation File   │
├──────────────────────────┼──────────────────────────┼───────────────────────┤
│ Serial COM1 Debug        │ Port 0x3F8 @ 115200 baud │ kernel/core/serial/   │
│ LAN Debug Transport      │ UDP Port 9999 (E1000)    │ kernel/debug/lan_debug│
│ Framebuffer Screenshots  │ UDP Port 9998 (SPMS)     │ kernel/debug/screensh.│
│ Host Test Controller     │ Python 3 Async Receiver  │ tools/capture_live_...│
│ First-Failure Latch      │ Spinlock-guarded Atomic  │ kernel/kernel.c       │
│ ABDE Diagnostic Renderer │ Direct Framebuffer Blit  │ kernel/display/abde/  │
└──────────────────────────┴──────────────────────────┴───────────────────────┘
```

---

## 3. TWO-CHANNEL MEDIA DEBUGGING SPECIFICATION

```
                           ┌───────────────────────────┐
                           │   MEDIA PIPELINE EVENT    │
                           └─────────────┬─────────────┘
                                         │
                 ┌───────────────────────┴───────────────────────┐
                 ▼                                               ▼
     ┌───────────────────────┐                       ┌───────────────────────┐
     │       CHANNEL A       │                       │       CHANNEL B       │
     │    LIVE SCREEN HUD    │                       │  PXE/LAN TELEMETRY    │
     │ (On-Screen Dashboard) │                       │ (Machine-Readable UDP)│
     └───────────┬───────────┘                       └───────────┬───────────┘
                 │                                               │
                 ▼                                               ▼
     ┌───────────────────────┐                       ┌───────────────────────┐
     │  Rendered into native │                       │ Serialized JSON/KV to │
     │  BOSurface or overlay │                       │ UDP 9999 + Screenshot │
     │  Directly on Monitor  │                       │ Stream on UDP 9998    │
     └───────────────────────┘                       └───────────────────────┘
```

### 3.1 Channel A: Live On-Screen Forensic HUD
ATOMS renders an on-screen diagnostic overlay (toggleable via `F12` or automatically invoked upon playback error).

#### Visual Specification:
```
+----------------------------------------------------------------------------+
| ATOMS MEDIA FORENSIC DEBUGGER v1.0                     [STATUS: FAIL (LOCKED)]|
+----------------------------------------------------------------------------+
| FILE: /TEST1/Dolby_Vision_AtmosHDR.mp4       PID: 7 (media_player.elf) RING: 3|
| CONTAINER: MP4 (ISO isom/mp42)               TRACKS: Video (AVC), Audio (EC3) |
+----------------------------------------------------------------------------+
| PIPELINE STAGE TELEMETRY:                                                  |
|   [PASS] 01. VFS / BOFS READ      : 38,125,940 bytes (AHCI SATA / USB MSC) |
|   [PASS] 02. MP4 DEMUXER          : 1,440 video samples, timescale 24,000  |
|   [PASS] 03. SPS PARSER           : Profile 100 (High), Level 4.0, 1920x1080|
|   [PASS] 04. PPS PARSER           : PPS ID 0, entropy_mode = 1 (CABAC)     |
|   [FAIL] 05. CABAC ENTROPY PARSER : Desync at MB 0, Slice 0 (IDR)          |
|   [WARN] 06. CONCEALMENT ENGINE   : 8,160 MBs errored -> solid grey fill   |
|   [PASS] 07. BOSPECTRA YUV->ARGB  : SIMD AVX2 active, 1920x1080 -> 960x540 |
|   [PASS] 08. BOSURFACE v2.5 SHM   : SHM ID 4, vaddr=0x70001000, stride=3840|
|   [HOLD] 09. FRAME SCHEDULER      : Sample 0, PTS=0 us, State=EARLY (+41ms)|
|   [BLOCKED] 10. BCM PRESENTATION  : Frame queue starved after Sample 0     |
+----------------------------------------------------------------------------+
| FIRST FAILURE ROOT-CAUSE LOCK:                                             |
|   STAGE   : H264_CABAC_DECODE                                              |
|   ERROR   : ERR_CABAC_UNSUPPORTED_SYNTAX (ret = 3)                         |
|   LOCATION: third_party/media/h264/h264bsd_cabac.c:284                     |
|   DETAIL  : Macroblock residual coefficients not implemented in stub       |
+----------------------------------------------------------------------------+
| SUBSYSTEM TELEMETRY:                                                       |
|   CPU: 2.8% | SIMD: AVX2 | GPU: SW Fallback | CLOCK: Monotonic TSC         |
|   QUEUE: 0/4 frames | AUDIO: EC-3 bypassed (No Decoder) | DROPPED: 0       |
+----------------------------------------------------------------------------+
```

---

### 3.2 Channel B: Machine-Readable PXE/LAN Telemetry
Over the existing `lan_debug` UDP port **9999**, ATOMS broadcasts machine-parseable telemetry packets whenever media events occur.

#### Packet Payload Format (Key-Value Schema):
```
[MEDIA_EVENT]
event_type=STAGE_FAILURE
timestamp_us=4219804
pid=7
ring=3
stage=H264_CABAC_DECODE
status=FAIL
error_code=0x80040003
error_name=ERR_CABAC_UNSUPPORTED_SYNTAX
file=Dolby_Vision_AtmosHDR.mp4
sample_index=0
pts_us=0
dts_us=0
width=1920
height=1080
num_err_mbs=8160
surface_id=4
surface_vaddr=0x0000700010000000
first_failure_locked=1
callsite=third_party/media/h264/h264bsd_cabac.c:284
```

---

## 4. AUTOMATIC FAILURE SCREENSHOT & REMOTE CAPTURE

### 4.1 Automatic Trigger on First Failure
When the media pipeline encounters an unrecoverable failure (e.g. decoder fatal error, surface mapping failure, or compositor disconnection):
1. **LATCH FAILURE**: The Media Debugger invokes `atoms_first_failure_record()`.
2. **FREEZE TELEMETRY**: Pipeline state is snapshotted into a static debug descriptor.
3. **TRIGGER SCREENSHOT**: The kernel invokes `atoms_screenshot_request(session_id)`.
4. **PACKETIZE & STREAM**:
   - `atoms_screenshot.c` reads the linear physical UEFI framebuffer.
   - Encapsulates pixel rows into Windows 32-bit BMP chunks with header magic `0x534D5053` ("SPMS").
   - Transmits UDP chunks across Ethernet to UDP port **9998**.
5. **HOST REASSEMBLY**: The host controller (`tools/capture_live_screen.py`) collects all chunks, verifies CRC, reassembles the BMP, converts it to PNG, and stores it in `artifacts/screenshots/`.

### 4.2 Remote Screenshot Request (Interactive Command)
External tools or AI agents can request an instantaneous visual snapshot at any time by sending a single UDP packet:

```
[HOST AGENT]                                      [ATOMS TARGET PC]
     │                                                    │
     │ ──── UDP Port 9999: "SCREENSHOT\n" ──────────────> │
     │                                                    │ Invokes atoms_screenshot_request()
     │ <─── UDP Port 9998: SCREENSHOT_CHUNK 0 (Header) ── │ Chunks linear framebuffer
     │ <─── UDP Port 9998: SCREENSHOT_CHUNK 1..N ──────── │ (1400 bytes/packet)
     │ <─── UDP Port 9998: SCREENSHOT_CHUNK LAST (Flag 2) │
     │                                                    │
     ▼                                                    ▼
Reassembles BMP -> Saves "artifacts/screenshots/live_hardware_TIMESTAMP.png"
```

---

## 5. REMOTE INTERACTIVE DEBUG COMMANDS

Via UDP Port **9999** (or COM1 serial shell), the following safe, read-only diagnostic commands are recognized:

| Command | Arguments | Response Description |
|:---|:---:|:---|
| `MEDIA STATUS` | None | Returns current playback state, PID, URI, container, and codec. |
| `MEDIA PIPELINE` | None | Dumps status of all 10 pipeline stages with pass/fail markers. |
| `MEDIA FRAME` | `[N]` | Dumps metrics for frame N (PTS, DTS, size, decode time, CRC). |
| `MEDIA SURFACE` | None | Dumps active BOSurface handle, resolution, stride, format, and mapped VAddr. |
| `MEDIA AUDIO` | None | Dumps PCM sample rate, channel count, FIFO occupancy, and HDA DMA status. |
| `MEDIA CLOCK` | None | Dumps monotonic audio clock, system clock, video PTS, and sync drift. |
| `MEDIA QUEUE` | None | Dumps presentation queue depth, peek frame PTS, and capacity. |
| `MEDIA FIRST_FAIL`| None | Dumps the locked first-failure record and callsite. |
| `SCREENSHOT` | None | Triggers immediate UDP 9998 full-screen framebuffer stream. |
| `TRACE LEVEL` | `0..5` | Dynamically updates runtime logging verbosity level. |

---

## 6. TRACE VERBOSITY LEVELS

To prevent log flooding and maintain smooth 24/60 FPS playback during normal execution:

- **LEVEL 0 (CRITICAL ERRORS ONLY)**:
  - Emits logs only on decode failures, surface allocation failures, or hardware DMA faults.
  - Zero performance impact.
- **LEVEL 1 (LIFECYCLE EVENTS)**:
  - Logs `MEDIA_OPEN`, `MEDIA_PLAY`, `MEDIA_PAUSE`, `MEDIA_STOP`, and `MEDIA_CLOSE`.
- **LEVEL 2 (PIPELINE STAGE MILESTONES)**:
  - Logs Demuxer ready, SPS/PPS parsed, audio stream started, surface created.
- **LEVEL 3 (FRAME & PACKET TELEMETRY)**:
  - Logs decoded frame count, presentation timestamps, queue depth, and drop events.
- **LEVEL 4 (SURFACE & MEMORY DETAILED)**:
  - Logs buffer addresses, stride calculations, color conversion benchmarks (SIMD vs scalar).
- **LEVEL 5 (DEEP FORENSIC TRACING)**:
  - Logs NAL unit byte offsets, slice headers, macroblock counters, and CRC calculations.

---

## 7. FIRST-FAILURE LATCH & STATE SNAPSHOT SCHEMA

In complex multi-threaded or pipeline systems, an initial failure triggers cascading secondary errors (e.g. CABAC failure -> DPB error -> frame queue starved -> surface cleared -> compositor blackout).

The **First-Failure Rule** guarantees that only the root cause is captured:

```c
typedef struct {
    uint32_t is_locked;             // 1 if first failure has been latched
    uint64_t timestamp_ticks;       // Monotonic system tick
    uint32_t pid;                   // Process ID
    uint32_t ring;                  // Execution Ring (0 or 3)
    char     subsystem[16];         // "H264_DEC", "BOSURFACE", "AUDIO"
    uint32_t error_code;            // Machine error code
    char     error_name[32];        // Human-readable string
    char     source_file[64];       // Source file path
    uint32_t source_line;           // Line number
    char     function_name[48];     // Function identifier
    uint64_t frame_index;           // Video frame or audio packet index
    int64_t  pts_us;                // Presentation timestamp
    uint64_t buffer_address;        // Relevant memory pointer
    uint32_t surface_id;            // BOSurface window ID
} AtomsFirstFailureRecord;
```

#### Latch Invariant:
```c
void atoms_media_record_failure(const char* subsys, uint32_t err, const char* name,
                                const char* file, uint32_t line, const char* func, ...) {
    static spinlock_t s_lock = SPINLOCK_INIT;
    spinlock_acquire(&s_lock);
    
    if (g_media_failure.is_locked) {
        // First failure already captured. Ignore subsequent cascading errors!
        spinlock_release(&s_lock);
        return;
    }
    
    // Latch initial root failure
    g_media_failure.is_locked = 1;
    // Populate snapshot fields...
    
    spinlock_release(&s_lock);
    
    // Trigger telemetry broadcast and automated screenshot
    debuglan_log_subsys(subsys, "FIRST_FAILURE_LOCKED: %s at %s:%d\n", name, file, line);
    atoms_screenshot_request(1);
}
```

---

## 8. PERFORMANCE & STABILITY GUARANTEES

1. **Non-Blocking Network Transmission**:
   - Telemetry packets are staged in a bounded 64-entry FIFO queue (`LAN_DEBUG_QUEUE_CAPACITY`).
   - Network transmission rate is strictly limited to 100 packets/second (`LAN_DEBUG_MAX_PACKETS_PER_SEC`) to prevent network flooding and NIC buffer exhaustion.
2. **Zero-Deadlock Invariant**:
   - Debug logging functions never acquire locks held by the media rendering thread or memory allocator.
3. **Memory Safety**:
   - No dynamic heap allocations (`malloc`/`free`) occur in the critical failure path. All debug descriptors and telemetry buffers are statically reserved in BSS.
4. **Graceful Degradation**:
   - If the Ethernet controller (E1000) or physical network link is disconnected, telemetry fails silently without stalling the media playback thread.

---
**Architecture Approved for Implementation Planning**:  
*ATOMS System Architecture Group — Phase 2 Ready*
