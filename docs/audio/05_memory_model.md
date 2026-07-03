# ATOMS OS Foundation v2 Audio Subsystem
## 05: Memory Model

The Audio Memory Model enforces strict ownership and cleanup rules to ensure that a continuous influx of audio data does not cause memory leaks or fragmentation, fulfilling the "Zero Leak Strategy" requirement.

### Buffer Types

#### 1. DMA Buffers
*   **Purpose**: Memory regions read directly by the audio hardware via Direct Memory Access.
*   **Allocation**: Must be allocated as physically contiguous memory blocks. As the OS does not currently have a fully fledged DMA API, this architecture mandates the future Physical Memory Manager (PMM) to support `pmm_alloc_contiguous()`.
*   **Lifetime**: Allocated once during driver `initialize()` and freed during `shutdown()`.

#### 2. Ring Buffers (Stream Buffers)
*   **Purpose**: Transferring PCM data from an application to the kernel without copying block-by-block synchronously.
*   **Allocation**: Kernel heap (`kmalloc`), typically sized around 16KB - 64KB per active stream.
*   **Lifetime**: Tied directly to the `AudioStream` object.

#### 3. Temporary Mix Buffers
*   **Purpose**: Used by the Audio Mixer to hold the summed 32-bit values before clipping them down to 16-bit for the hardware.
*   **Allocation**: Static or pre-allocated on the kernel heap during subsystem initialization. Never dynamically allocated per-tick to avoid heap fragmentation and latency.

### Ownership & Cleanup Rules

*   **App to Kernel Boundary**: When an app writes to an audio stream, the kernel copies the data into the Stream Ring Buffer. The ring buffer is exclusively owned by the Kernel Audio Core.
*   **Zero Leak Strategy (Process Death)**: Every `AudioStream` structure is tagged with the owning Process ID. If the OS Process Manager detects a crashed or terminated process, it invokes the `audio_core_cleanup_pid(pid)` function. This function halts the stream, detaches it from the mixer, and frees the associated Ring Buffer.
*   **Explicit Teardown**: Apps that close streams properly will trigger a synchronous free of the stream resources.
