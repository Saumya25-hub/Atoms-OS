# ATOMS OS Foundation v2 Audio Subsystem
## 07: Mixer Design

The Audio Mixer is a purely software-based component residing in the kernel. Its job is to take multiple independent PCM streams and combine them into a single continuous stream for the hardware driver.

### Responsibilities
1.  **Summing**: Adding the waveforms of multiple streams together.
2.  **Volume Scaling**: Applying per-stream volume controls.
3.  **Clipping**: Preventing integer overflow distortion when loud sounds are summed.
4.  **Format Conversion**: (Future Expansion) Resampling streams that don't match the hardware's native sample rate.

### Mixing Algorithm

The mixer operates on a periodic tick (driven by the audio hardware interrupt).

1.  **Locking**: The mixer acquires a spinlock on the active stream list to prevent streams from being destroyed while mixing.
2.  **Accumulation**: For each sample to be generated, the mixer reads one 16-bit sample from every active stream's ring buffer.
3.  **Scaling**: It multiplies each 16-bit sample by its stream's volume coefficient (0.0 to 1.0, represented as fixed-point math to avoid FPU context overhead in the kernel).
4.  **Summation**: It adds the scaled samples into a 32-bit signed integer accumulator (`int32_t`) to prevent overflow.
5.  **Clipping**: After all streams are summed for that sample, the 32-bit value is clamped to the 16-bit range (`-32768` to `32767`).
6.  **Output**: The resulting 16-bit sample is written to the DMA ring buffer.

### Pseudocode Core Loop
```c
for (int i = 0; i < samples_needed; i++) {
    int32_t mixed_sample = 0;
    
    for (each stream in active_streams) {
        if (ring_buffer_has_data(stream)) {
            int16_t sample = ring_buffer_read(stream);
            mixed_sample += (sample * stream->volume) / 255;
        }
    }
    
    // Hard Clipping
    if (mixed_sample > 32767) mixed_sample = 32767;
    if (mixed_sample < -32768) mixed_sample = -32768;
    
    dma_buffer[i] = (int16_t)mixed_sample;
}
```

### CPU Usage Optimization
To meet the "Minimal CPU usage" target, the mixer avoids floating-point operations entirely. It uses integer arithmetic and bit-shifting. If only one stream is active, the mixer bypasses the accumulation phase and performs a direct memory copy (memcpy/DMA) scaled by volume.
