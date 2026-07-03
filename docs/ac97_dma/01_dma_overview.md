# AC'97 DMA Engine - Overview

The ATOMS OS Phase 7.6.1 establishes the hardware transport layer using Bus Master DMA.

## Purpose
While the previous phases constructed the physical codec detection, this phase constructs the memory pipeline. The AC'97 controller uses Scatter-Gather DMA to autonomously stream audio samples from system memory (RAM) to the Codec without utilizing CPU cycles. 

## Scope
This phase strictly implements the memory allocation, state machine, and hardware initialization for the DMA controller. We do not yet implement the playback loop or the software mixer integration. By isolating the foundation, we ensure memory safety and zero leaks before live streaming begins.
