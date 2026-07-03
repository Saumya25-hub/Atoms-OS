# ATOMS OS Foundation v2 Audio Subsystem
## 06: Buffer Design

The subsystem relies heavily on Ring Buffers (Circular Buffers) to manage the asynchronous nature of audio production (by the app) and audio consumption (by the hardware).

### The Ring Buffer Structure

```c
typedef struct {
    uint8_t* data;
    size_t capacity;
    volatile size_t head; // Written to by producer
    volatile size_t tail; // Read from by consumer
} AudioRingBuffer;
```

### Dual-Ring Architecture

#### 1. App-to-Kernel (Software) Ring Buffer
Each active audio stream has its own ring buffer.
*   **Producer**: The userspace application (via Horse Engine syscalls).
*   **Consumer**: The Kernel Audio Mixer.
*   **Concurrency**: Designed to be lock-free for single-producer/single-consumer setups, though a lightweight spinlock can be used if multiple threads within the app attempt to write to the same stream simultaneously.

#### 2. Kernel-to-Hardware (DMA) Ring Buffer
The final mixed output buffer.
*   **Producer**: The Kernel Audio Mixer.
*   **Consumer**: The Audio Hardware (via DMA).
*   **Concurrency**: Strictly hardware-managed. The hardware raises an interrupt when it crosses a half-way threshold, prompting the Mixer to fill the consumed half while the hardware reads the other half (Double Buffering / Ping-Pong Buffer).

### Buffer Sizing and Latency
*   **Size**: A typical DMA ring buffer might be 16KB. At 44.1kHz, 16-bit Stereo, this equates to roughly ~90ms of audio. 
*   **Interrupt Frequency**: Firing an interrupt at the half-way point (8KB) yields an interrupt every ~45ms. 
*   **Low Latency Target**: For applications requiring extremely low latency (e.g., DOOM), the buffer size can be negotiated smaller, resulting in faster interrupts but higher CPU overhead.

### Overruns and Underruns
*   **Underrun**: The app fails to write data fast enough. The tail catches up to the head. The Mixer will output silence for this stream.
*   **Overrun**: The app writes too much data. The head catches up to the tail. The `stream()` syscall will return an error (or block, depending on the requested mode) indicating the buffer is full.
