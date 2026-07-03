# AC'97 DMA Engine - Memory Architecture

The DMA Engine requires exact synchronization between Virtual Memory (used by the kernel/software) and Physical Memory (used by the hardware controller).

## The Manager
The `Ac97DmaManager` struct maintains both sets of addresses:
- `pcm_buffer` (Virtual) and `pcm_buffer_phys` (Physical)
- `bdl` (Virtual) and `bdl_phys` (Physical)

## Allocation Strategy
1. The engine requests memory via the standard `kmalloc`.
2. It then invokes the ATOMS Virtual Memory Manager (`vmm_get_physical_address`) to map the active `PML4` entry into a 32-bit physical address.
3. This 32-bit physical address is safely programmed into the controller's Base Address registers.

## Lifetime
Memory is aggressively freed during `ac97_dma_shutdown()`. Because the kernel might dynamically allocate audio streams in the future, the DMA engine guarantees that destroyed streams return 100% of their physical memory to the heap.
