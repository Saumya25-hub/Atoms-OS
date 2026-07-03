# AC'97 Driver Foundation - Overview

The ATOMS OS Phase 7.5 establishes the critical AC'97 hardware driver foundation.

## Purpose
While the previous phases (Audio Core, PCM Engine, Software Mixer) constructed the software rendering engine, the AC'97 Driver bridges the gap between software and physical sound output. The AC'97 (Audio Codec '97) is an Intel specification standardizing audio controller behavior.

## Scope of Foundation
This phase strictly focuses on **Hardware Detection and State Initialization**.
We intentionally exclude DMA configuration, buffer descriptor allocations, and actual PCM playback. These belong to Phase 7.6. 

By separating initialization from playback, we ensure that:
1. The kernel can safely identify the hardware on the PCI bus without crashing.
2. The Codec can be correctly cold-reset and placed into a stable `Ready` state.
3. The register communication layer is completely fault-tolerant before any high-speed DMA transfers are attempted.
