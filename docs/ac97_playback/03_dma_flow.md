# AC'97 Playback - DMA Flow

The AC'97 DMA flow is designed around the continuous population and consumption of the Ring Buffer.

## Pre-Fill (Start)
When `ac97_playback_start()` is called:
1. The 32 descriptors (representing 64KB total) are completely pre-filled by the Software Mixer.
2. The Last Valid Index (`LVI`) is set to `31` (the end of the ring).
3. The DMA Controller is ordered to start (`PO_CR` RPBM bit).

## Continuous Flow (Update)
During playback, the engine continually polls the hardware. 
When the hardware finishes a chunk (e.g., descriptor `0`), it increments its internal Current Index Value (`CIV`). 
The software detects this delta and immediately refills the data in chunk `0` via the Software Mixer. It then advances the `LVI` to let the hardware know chunk `0` is valid again, ensuring the ring buffer never empties.
