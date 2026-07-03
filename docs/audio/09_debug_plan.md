# ATOMS OS Foundation v2 Audio Subsystem
## 09: Debug Plan

To ensure the Audio Subsystem operates smoothly in real-world scenarios, extensive telemetry will be built into the core. This allows the OS developers and users to diagnose stutters, latency issues, and memory leaks.

### Telemetry Counters & Metrics

1.  **Playback Latency**
    *   Measured as the time delta between an app invoking `horse_audio_play()` and the data actually hitting the hardware DMA buffer.
    *   *Target*: Monitor for spikes > 50ms.

2.  **Underruns / Overruns**
    *   *Underrun Counter*: Incremented when the mixer attempts to read an app's stream but the ring buffer is empty (causes stutter).
    *   *Overrun Counter*: Incremented when an app attempts to write to a full ring buffer.

3.  **Buffer Usage Metrics**
    *   Real-time reporting of the `head` and `tail` distances for all active ring buffers. Helps identify apps that are struggling to feed data fast enough.

4.  **Mixer Load (CPU Time)**
    *   Measured via the CPU timestamp counter (RDTSC on x86) before and after the mixer accumulation loop. This ensures the mixer isn't monopolizing the kernel.

5.  **Driver & Hardware Status**
    *   *Interrupt Count*: Number of times the hardware has requested more data. If this stops incrementing, the DMA engine has stalled.
    *   *DMA Status Register*: Polled occasionally for error flags.

6.  **Dropped Frames**
    *   Counter for PCM samples discarded because the hardware couldn't accept them in time (usually an OS scheduling issue).

7.  **Active Streams & Sounds**
    *   A live view of how many `AudioStream` objects are currently allocated, helping verify that short sounds (like UI clicks) are correctly destroyed when finished.

8.  **Memory Usage**
    *   Total bytes allocated on the kernel heap for Audio Ring Buffers. Crucial for verifying the Zero Leak Strategy.

### Sysfs / Debugfs Exposure
These metrics will be exposed via read-only files in the virtual filesystem (e.g., `/sys/audio/stats`), allowing standard user-space tools to poll and graph audio health.
