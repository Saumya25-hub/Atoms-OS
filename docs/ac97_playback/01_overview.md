# AC'97 Playback - Overview

This phase connects the Software Mixer to the AC'97 DMA Engine.

## Purpose
The DMA Engine (Phase 7.6.1) provided a robust memory allocation and hardware programming layer. However, it did not provide the actual PCM data or manage the ongoing ring buffer progress. This playback layer bridges the gap, actively monitoring the hardware's position in the buffer (CIV) and continually fetching new samples from the Software Mixer to refill the buffer chunks that have already been played.

## Approach
Because interrupt handling is relegated to a future phase, this engine employs a strict **Polling** methodology. A continuous update loop reads the DMA status registers and manages the descriptor rotation manually, guaranteeing safe, deterministic data flow without preemptive interrupts.
