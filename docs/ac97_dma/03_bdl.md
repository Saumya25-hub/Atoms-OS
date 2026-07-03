# AC'97 DMA Engine - Buffer Descriptor List (BDL)

The AC'97 Controller relies on a Ring Buffer composed of 32 physical descriptors.

## The BDL Entry
Each entry in the BDL is strictly 8 bytes, defined by the AC'97 specification:
```c
typedef struct {
    uint32_t buffer_phys_addr; // Physical address of the PCM chunk
    uint16_t length;           // Length in SAMPLES (not bytes)
    uint16_t flags;            // Interrupt & Policy flags
} Ac97BdlEntry;
```

## Descriptor Mapping
During `ac97_dma_prepare()`, the single contiguous PCM Buffer is divided evenly across all 32 entries. For example, a 64KB buffer is split into 32 chunks of 2KB each. 
Because the Intel spec requires length to be denoted in **samples** (16-bit words), the 2KB chunk is registered as `1024` length in the descriptor.

## Flags
- `AC97_BDL_FLAG_IOC` (Bit 15): Interrupt On Completion. Tells the hardware to fire an IRQ when this chunk finishes playing.
- `AC97_BDL_FLAG_BUP` (Bit 14): Buffer Underrun Policy. Prevents the hardware from repeating the last valid chunk if the CPU fails to deliver new data in time.
