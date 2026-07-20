# SignaturesOS Audio Micro-Stutter — QEMU Visible vs Minimized Forensic Audit

**Date:** July 21, 2026
**Observation:** Music plays with periodic micro-stutters when QEMU window is visible. Minimizing QEMU = perfectly smooth audio. Restoring = stutters return.
**Status:** READ-ONLY AUDIT COMPLETE → Controlled A/B Test Build implementation follows.

---

## PHASE 1 — OBSERVER EFFECT: All Periodic Printing During Audio Playback

### ⚠️ CRITICAL FINDING: Serial Output Is A Massive CPU Time Sink

**Serial port configuration** — [display.c:31](file:///D:/Signatures_OS/kernel/drivers/display/display.c#L31):
```c
io_out8(SERIAL_PORT + 0, 0x03);    // Divisor 3 → 38400 baud
```

At **38400 baud**, each character takes:
$$\frac{1}{38400} \times 10\text{ bits} = \mathbf{260\,\mu s\text{ per character}}$$

Every `serial_write()` / `serial_write_direct()` / `display_print()` call **busy-waits** on the UART Transmit Holding Register:
```c
// display.c:43 — BLOCKING busy-wait per character
while ((io_in8(SERIAL_PORT + 5) & 0x20) == 0);
io_out8(SERIAL_PORT, c);
```

> [!CAUTION]
> Interrupts remain ENABLED during this busy-wait, so IRQ 0 (audio realtime pump) is NOT blocked.
> However, the **calling task/thread cannot yield or do other work** — it burns its entire CPU quantum spinning on I/O port reads.

---

### Telemetry Source #1: `print_1sec_telemetry()` — THE ELEPHANT

| Property | Value | Proof |
|:---|:---|:---|
| **File** | [kernel.c:368-583](file:///D:/Signatures_OS/kernel/kernel.c#L368-L583) | — |
| **Called from** | Main kernel loop, line 1013: `print_1sec_telemetry()` | Every main loop iteration |
| **Fires every** | 1 second (1000 tick check) | [kernel.c:372](file:///D:/Signatures_OS/kernel/kernel.c#L372) |
| **Thread** | **Boot Task / Main Loop** (NOT AudioSvc) | — |
| **Output type** | `serial_write_direct()` — blocking serial | — |
| **Character count** | **~2,500 characters** (75+ lines of telemetry) | Counted from lines 430-582 |
| **Blocking time** | 2,500 × 260 µs = **~650 ms blocking** per dump! | Derived |

> [!WARNING]
> **650 ms of CPU burn every 1 second!** The boot task spends 65% of its time busy-waiting on serial output. During this time:
> - **A.** Boot task cannot yield to AudioSvc → AudioSvc starved
> - **B.** IRQ 0 CAN fire → `audio_realtime_worker_pump()` still runs (consumer OK)
> - **C.** AudioSvc producer CANNOT run (scheduler won't switch while boot task has quantum)
> - **D.** AC97 realtime update CAN run (inside IRQ 0 handler, pre-scheduler)

**But:** The main loop only yields to AudioSvc when `wait_ms > 2` and `scheduler_get_task_count() > 0` ([kernel.c:1074-1078](file:///D:/Signatures_OS/kernel/kernel.c#L1074-L1078)). If the boot task is stuck in `print_1sec_telemetry()` for 650ms, it **cannot reach the yield point**, starving AudioSvc for up to 650ms.

---

### Telemetry Source #2: `ac97_playback_status()` — Every 5 Seconds from AudioSvc

| Property | Value | Proof |
|:---|:---|:---|
| **File** | [ac97_playback.c:437-461](file:///D:/Signatures_OS/kernel/audio/drivers/ac97/ac97_playback.c#L437-L461) | — |
| **Called from** | `audio_service_entry()` at [kernel.c:356](file:///D:/Signatures_OS/kernel/kernel.c#L356) | `if (!g_RAM_Only_Test_Active)` |
| **Fires every** | 5 seconds (250 × 20ms) | [kernel.c:330](file:///D:/Signatures_OS/kernel/kernel.c#L330) |
| **Thread** | **AudioSvc task** (the audio producer!) | — |
| **Output type** | `display_print()` — serial + optional GUI console | — |
| **Character count** | ~500 chars (status) + ~200 chars (player telemetry) = **~700 characters** | Counted |
| **Blocking time** | 700 × 260 µs = **~182 ms blocking** per dump | Derived |

> [!CAUTION]
> **This is the producer thread burning 182ms printing telemetry instead of refilling the ring buffer.** During this time, the ring buffer drains at 192,000 bytes/sec = **34,816 bytes lost** in 182ms. That's 2× descriptor size — enough to cause a CIV jump of 2.

---

### Telemetry Source #3: `flight_recorder_dump()` — Event-Triggered from IRQ Context

| Property | Value | Proof |
|:---|:---|:---|
| **File** | [ac97_playback.c:38-64](file:///D:/Signatures_OS/kernel/audio/drivers/ac97/ac97_playback.c#L38-L64) | — |
| **Triggered by** | DCH halt, ring starvation, CIV jump >2, mixer short fill, gap >50ms | [ac97_playback.c:394-401](file:///D:/Signatures_OS/kernel/audio/drivers/ac97/ac97_playback.c#L394-L401) |
| **Thread** | **IRQ 0 handler** (inside `ac97_playback_update()`) | — |
| **Output type** | `display_print()` — serial output FROM IRQ CONTEXT | — |
| **Character count** | 128 entries × ~120 chars = **~15,360 characters** | Counted |
| **Blocking time** | 15,360 × 260 µs = **~3.99 seconds blocking** inside IRQ! | Derived |

> [!CAUTION]
> When triggered, this **freezes the entire system for ~4 seconds** inside the IRQ handler. No scheduling, no audio, no input. However, it only fires once (`g_flight_frozen = true`).

---

### Telemetry Source #4: `audio_test_mode_telemetry_tick()` — Only in ATM Mode

| Property | Value |
|:---|:---|
| **Active** | Only when `AUDIO_TEST_MODE_ENABLED` is defined |
| **Irrelevant** | Not active in Normal Desktop Boot mode |

---

## PHASE 2 — IS THE RING ACTUALLY STARVING?

### Existing Flight Recorder Already Records CIV Transitions

The existing `StutterFlightSample` structure at [ac97_playback.c:16-31](file:///D:/Signatures_OS/kernel/audio/drivers/ac97/ac97_playback.c#L16-L31) already captures:

| Field | Present | User's Requirement |
|:---|:---|:---|
| `timestamp_us` | ✅ | ✅ |
| `previous_civ` | ✅ | ✅ |
| `current_civ` | ✅ | ✅ |
| `lvi` | ✅ | ✅ |
| `civ_jump` | ✅ | ✅ |
| `ring_avail_before` | ✅ | ✅ |
| `ring_avail_after` | ✅ | ✅ |
| `mixer_returned` | ✅ | ✅ |
| `gap_since_last_us` | ✅ | ✅ |
| `producer_refills` | ✅ | ✅ |
| `producer_bytes` | ✅ | ✅ |
| `silence_total` | ✅ | ✅ |
| `dch` | ✅ | ✅ |

**HOWEVER:** The existing recorder has two fatal problems:

1. **It triggers `flight_recorder_dump()` (serial print) on first failure** → observer effect
2. **It freezes after first dump** (`g_flight_frozen = true`) → captures only first event

**WHAT'S MISSING:** Aggregate statistics that accumulate silently:
- `RingMinAvailable`, `RingMaxAvailable`, `RingAverageAvailable`
- `RingBelow4KCount`, `RingBelow16KCount`, `RingBelow64KCount`
- `MixerSilenceEvents`, `MixerSilenceBytes`
- `MaxProducerServiceGapUs`, `MaxAC97UpdateGapUs`, `MaxCIVJump`
- Forensic marker support for A/B/C test phases

---

## PHASE 3 — PRODUCER BUFFERING POLICY: MATHEMATICAL AUDIT

### The Exact Policy ([audio_player.c:300-448](file:///D:/Signatures_OS/kernel/audio/session/audio_player.c#L300-L448))

```
Constants:
  PRODUCER_CHUNK_SIZE             = 16,384 bytes  (16 KB)
  PRODUCER_HIGH_WATERMARK_PCT     = 80%
  PRODUCER_LOW_WATERMARK_PCT      = 40%
  PRODUCER_CRITICAL_WATERMARK_PCT = 20%
  PRODUCER_REFILL_TARGET_PCT      = 90%
```

### Decision Logic Per Service Call

| Ring Occupancy | max_chunks | Bytes Written | Time to Read (VFS) |
|:---|:---|:---|:---|
| ≥ 80% | **0** (skipped entirely) | 0 bytes | 0 ms |
| 40-79% | 2 | 32,768 bytes | ~36-64 ms |
| 20-39% | 4 | 65,536 bytes | ~72-128 ms |
| < 20% | 8 | 131,072 bytes | ~144-256 ms |
| Initial prefill | 16 | 262,144 bytes | ~288-512 ms |

### The Critical Math

| Metric | Value | Proof |
|:---|:---|:---|
| Ring capacity | 262,144 bytes | [audio_stream.c:5](file:///D:/Signatures_OS/kernel/audio/streams/audio_stream.c#L5) |
| Audio drain rate | 192,000 bytes/sec | 48kHz × 2ch × 2bytes |
| AudioSvc wake interval | 20 ms | [kernel.c:360](file:///D:/Signatures_OS/kernel/kernel.c#L360) |
| Audio drained per 20ms | 3,840 bytes | 192000 × 0.020 |
| **80% threshold** | **209,715 bytes** | 262144 × 0.80 |
| **90% target** | **235,930 bytes** | 262144 × 0.90 |

### VERDICT: Producer Policy is Correctly Designed BUT...

The watermark policy itself is mathematically sound — at 80% occupancy, it writes 32KB which restores the buffer to ~92%. The **critical weakness** is:

> [!IMPORTANT]
> **The producer can ONLY run when AudioSvc gets CPU time from the scheduler.**
> If the boot task is stuck in `print_1sec_telemetry()` for 650ms, AudioSvc doesn't run for 650ms.
> In 650ms, the ring drains: 192,000 × 0.650 = **124,800 bytes** (47.6% of capacity).
> Buffer drops from 90% → ~42% in one telemetry dump cycle.
> If TWO back-to-back scheduling gaps overlap, buffer reaches 0%.

### Producer Throughput vs Consumption

| Path | Throughput |
|:---|:---|
| **Consumer** (192,000 B/s) | Fixed, hardware-driven |
| **Producer** best case (2 chunks/20ms) | 1,638,400 B/s (8.5x headroom) |
| **Producer** worst case (stalled 650ms then 8 chunks) | 131,072 bytes / 670ms = 195,631 B/s (**barely 1.02x!**) |

> [!WARNING]
> When the boot task consumes 650ms for serial telemetry, the producer's effective throughput drops to **barely above consumption rate**. Any additional jitter (QEMU visible rendering, ATA I/O) tips it below.

---

## PHASE 4 — CAN DISPLAY LOAD DIRECTLY STARVE AUDIO?

### Execution Ownership Map

```
IRQ 0 (1000 Hz) — CANNOT BE STARVED BY DISPLAY LOAD
├── system_ticks++
├── audio_realtime_worker_pump()     ← CONSUMER: always runs
│   └── ac97_playback_update()       ← reads CIV, refills DMA descriptors
├── context_save_state()
├── scheduler_on_tick()              ← may switch tasks
└── context_restore_state()

AudioSvc Task (scheduler-dependent) — CAN BE STARVED
├── audio_player_update()            ← PRODUCER: reads VFS, fills ring buffer
├── ac97_playback_status()           ← 5-sec telemetry (182ms serial block)
└── scheduler_sleep(20)

Boot Task / Main Loop (highest priority) — DOES THE STARVING
├── print_1sec_telemetry()           ← 650ms serial block every 1 second!!
├── xhci_poll()
├── input_adapter_pump()
├── BWE_PumpEvents()
├── BSPE_CursorPresenter_PumpFastPath()
├── BOHeart_Pulse(hw_fb)             ← compositor + presentation
└── scheduler_yield()                ← ONLY yields here, if it reaches here
```

### Long CLI/Interrupt-Disabled Sections Found

| Location | Duration | Blocks IRQ 0? | Blocks AudioSvc? |
|:---|:---|:---|:---|
| [ata.c:83-85](file:///D:/Signatures_OS/kernel/drivers/storage_legacy/storage/src/ata.c#L83-L85) `cli; rep insw; sti` | ~13-26 µs per sector | **YES** | YES |
| [scheduler.c:264-270](file:///D:/Signatures_OS/kernel/core/scheduler/src/scheduler.c#L264-L270) `cli` around queue ops | ~1-5 µs | YES briefly | YES briefly |
| [heap.c:305-339](file:///D:/Signatures_OS/kernel/core/memory/heap/src/heap.c#L305-L339) `cli` during `heap_lock()` | ~1-10 µs | YES briefly | YES briefly |
| **BSPE / Compositor** | **ZERO `cli` found** | NO | NO |
| **Display/Serial** | No `cli`, but polling spinlock | NO | Task-level block |

### WHY QEMU VISIBLE = STUTTER, MINIMIZED = SMOOTH

```
QEMU Visible:
├── Host must render guest framebuffer at ~60 FPS
│   └── QEMU host thread spends CPU time on VGA register reads,
│       framebuffer scaling, and SDL/GTK rendering
├── QEMU vCPU thread gets LESS host CPU time
│   └── Guest timer IRQ delivery becomes jittery
│       └── CIV transitions get serviced late
│       └── Producer service gaps increase
├── Boot task's print_1sec_telemetry() AMPLIFIES the problem
│   └── 650ms of serial busy-wait burns vCPU time
│       └── AudioSvc gets even LESS time
│           └── Ring buffer drains → micro-stutter

QEMU Minimized:
├── Host SKIPS framebuffer rendering entirely
│   └── QEMU vCPU thread gets MORE host CPU time
│       └── Timer IRQ delivery is consistent
│       └── Scheduling is fair
│       └── Even with telemetry overhead, enough margin exists
│           └── Audio smooth
```

---

## PHASE 5 — CONTROLLED A/B TEST BUILD SPECIFICATION

### Changes Required

**A. Disable `print_1sec_telemetry()` during active playback:**
- Add guard: if `audio_player_is_playing()` → skip serial dump, counters still update in RAM

**B. Disable `ac97_playback_status()` during active playback:**
- The `audio_service_entry()` 5-second telemetry must be silent during playback

**C. Disable `flight_recorder_dump()` auto-trigger:**
- Remove the auto-print trigger conditions in `ac97_playback_update()`
- Keep flight buffer recording silently

**D. Add silent aggregate statistics:**
- `RingMinAvailable`, `RingBelow4KCount`, `RingBelow16KCount`, `RingBelow64KCount`
- `MixerSilenceEvents` (already: `g_MixerShortFills`)
- `MaxCIVJump` (already: `g_CIVMaxJump`)
- `MaxAC97UpdateGapUs` (already: `g_AudioWorkerMaxGapUs`)
- `MaxProducerServiceGapMs` (already: `g_ProducerMaxServiceGapMs`)

**E. Add forensic marker support:**
- Key press (e.g., F9) records `MARKER_A`, `MARKER_B`, `MARKER_C` timestamps into RAM

**F. One-time summary dump after playback stops:**
- Print accumulated stats ONCE when `ac97_playback_stop()` is called

### Decision Tree After Test

| Case | Evidence | Root Cause |
|:---|:---|:---|
| **A** | Stutter disappears after telemetry disabled | Observer effect / serial blocking proven |
| **B** | Visible: RingMin collapses, MixerSilence ↑; Minimized: healthy | Producer starvation / insufficient buffering |
| **C** | Ring healthy but MaxCIVJump ↑ while visible | AC97 servicing delayed by host scheduling |
| **D** | All guest metrics healthy, stutter only audible while visible | QEMU host audio/video scheduling interaction |
