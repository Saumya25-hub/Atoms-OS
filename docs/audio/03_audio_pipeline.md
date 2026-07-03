# ATOMS OS Foundation v2 Audio Subsystem
## 03: Audio Pipeline

The playback pipeline is designed to minimize latency while ensuring stability. It follows a strict directional flow from userspace down to the hardware speaker.

### Complete Workflow

1.  **Application**: 
    The userspace application decides to play a sound (e.g., DOOM gun firing, or background music). It loads the asset (WAV/MP3).

2.  **Userspace Decoder**: 
    The application utilizes `lib/media` to decode the compressed or containerized format into raw PCM frames.

3.  **Horse Engine API**: 
    The application invokes `horse_audio_play()` or `horse_audio_stream()` with the PCM data.

4.  **Syscall Boundary**: 
    The Horse Engine issues an interrupt/syscall crossing into kernel space, passing a pointer to the PCM data and length.

5.  **Audio Core & Stream Engine**: 
    The Kernel Audio Core creates an `AudioStream` object linked to the calling process. It allocates a Ring Buffer for this stream and copies the initial PCM data from userspace.

6.  **Audio Mixer**: 
    During the kernel's audio worker thread cycle, the Mixer reads from all active `AudioStream` ring buffers. It sums the samples, applies volume scaling, and clips the output to prevent distortion.

7.  **Device Ring Buffer / DMA Buffer**: 
    The mixed output is pushed into the primary hardware Ring Buffer (or directly into the DMA buffer if supported).

8.  **Audio Driver**: 
    The driver configures the hardware to read from the specified memory location and initiates playback.

9.  **Hardware Interrupts**: 
    As the hardware plays the audio, it consumes the DMA buffer. When it reaches a low-water mark (e.g., half empty), it triggers an IRQ.

10. **Interrupt Handler**: 
    The OS Interrupt System catches the IRQ and calls the audio driver's ISR. The ISR acknowledges the hardware interrupt and signals the Audio Worker Thread to wake up and mix more data.

11. **Speaker**: 
    The hardware DAC converts the digital PCM data into analog signals, resulting in audible sound.

### Pipeline Guarantees
*   **Non-blocking**: Userspace apps will not block waiting for audio hardware.
*   **Isolation**: A crashing app will have its pipeline torn down securely by the OS process manager, halting its sound without affecting other active streams.
