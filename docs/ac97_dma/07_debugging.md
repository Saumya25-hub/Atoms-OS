# AC'97 DMA Engine - Debugging & Risks

## Known Risks

### IOMMU and Identity Mapping
Currently, the OS translates heap virtual addresses into physical addresses using `vmm_get_physical_address()`. This assumes the physical address returned is contiguous and identical to what the hardware sees. If an IOMMU is enabled in the future, a dedicated DMA bouncing layer or IOMMU translation table must be used.

### The 16-bit Length Field
The BDL `length` field is strictly 16-bit (max `65,535`). Furthermore, it represents SAMPLES, not bytes. 
For a 64KB buffer across 32 descriptors, each descriptor handles `2048` bytes (`1024` samples). This comfortably fits inside the 16-bit limit. However, if the buffer size is drastically increased, the length field could overflow, causing the DMA controller to read random memory.

## Debugging Workflow
If DMA hangs:
1. Verify `PO_SR` is returning `0x02` (Halted) after a reset.
2. Verify `vmm_get_physical_address()` didn't return `0x0`.
3. Check `qemu.log` telemetry to ensure the `bdl_phys` address aligns with expectations.
