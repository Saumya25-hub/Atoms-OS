# Software Mixer Engine - Mix Math

The entire foundation of the software mixer rests on `audio_mix_math.c`.

## Integer Summation
Because we process 16-bit signed audio (`INT16_MAX` = 32767), adding multiple waveforms together can easily exceed this limit.

To prevent silent buffer corruption and extreme digital distortion (integer wrapping), all summations take place inside a 32-bit accumulator buffer (`int32_t`).

```c
void audio_math_mix_16(int32_t* accum, const int16_t* source, size_t samples) {
    for (size_t i = 0; i < samples; i++) {
        accum[i] += source[i]; // Safely exceeds 32767
    }
}
```

## Normalization (Clamping)
Before the 32-bit mixed data can be safely cast back to the 16-bit output buffer, the engine clamps the amplitude.
```c
if (val > 32767) {
    dest[i] = 32767; // Hard clip
    clips++;
} else if (val < -32768) {
    dest[i] = -32768; // Hard clip
    clips++;
}
```
While a soft-knee compressor would reduce distortion, hard clipping is extremely CPU-efficient and guarantees absolute hardware protection. Future revisions may include a software limiter.
