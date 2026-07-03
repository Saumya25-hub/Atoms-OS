# AC'97 DMA Engine - Testing

Validation of the DMA Engine in Phase 7.6.1 is based on a rigorous stress test designed to expose memory leaks.

## The Stress Test
Because the DMA Engine will eventually allocate and destroy streams dynamically, we must guarantee that it releases 100% of its physical memory. 
The driver implements a 1000-iteration loop:
1. `ac97_dma_prepare(65536)` (Allocates a 64KB PCM buffer and a 256-byte BDL).
2. Translates them to physical addresses.
3. Programs the hardware.
4. `ac97_dma_shutdown()` (Stops the hardware and `kfree`s the virtual allocations).

## Results
The QEMU log confirmed:
```text
[AC97 DMA] Starting Stress Test (1000 Iterations)...
[AC97 DMA] STRESS TEST: 1000 Iterations PASS
[AC97 DMA] Memory Leaks: 0 bytes
```
If there was a leak, the kernel heap would have exhausted after roughly `64 MB` of allocations, throwing a panic. The test easily survived the equivalent of 64MB of churn.
