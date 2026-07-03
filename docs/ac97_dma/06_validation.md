# AC'97 DMA Engine - Validation & Safety

The driver employs multiple defensive measures to ensure the DMA controller behaves deterministically.

## Physical Address Read-Back
After the VMM returns the physical address of the BDL array, the driver writes it to the `PO_BDBAR` register over I/O ports. 
To validate the bus, the driver immediately reads the `PO_BDBAR` register back. If the value does not exactly match what was written, the State Machine aborts to `ERROR`. This guarantees that the hardware will not attempt to fetch memory from an unmapped address space.

## Descriptor Division
The main PCM ring buffer is divided symmetrically across all 32 descriptors. 
The driver forces the buffer chunks to be an even number of bytes, ensuring sample alignment (each sample is 16-bits). If a chunk has an odd number of bytes, the hardware would desync channels (Left would become Right).
