# 01 - Audio Pipeline Signal Flow

## Goal
Document the end-to-end signal flow of the ATOMS OS Audio Subsystem, verifying that a generated signal mathematically passes through every subsystem untouched until the final hardware stage.

## Pipeline Architecture
1. **Application (Test Tone)**: Generates a mathematically perfect 440Hz Sine wave.
2. **Audio Core (Stream Registry)**: Maps the continuous waveform into a ring buffer.
3. **Software Mixer**: Reads the ring buffer, applies 16-bit integer mixing and clipping algorithms.
4. **DMA BDL (Buffer Descriptor List)**: The physical memory layout translating the PCM chunks for hardware.
5. **AC'97 Controller**: The hardware controller reading the Bus Master data.

## Evidence
- Signal origin: `audio_test_tone.c` at 440Hz, 16-bit, 48000Hz.
- Stream Integrity: Verified by the PCM Engine test.
- Mixer Output: Verified by the Mixer Engine test.
- Hardware Transport: Verified by the DMA Checksums.
- Physical Playback: Verified by QEMU's `wav` backend capture script (`verify_wav.ps1`).
