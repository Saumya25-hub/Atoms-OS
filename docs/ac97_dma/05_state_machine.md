# AC'97 DMA Engine - State Machine

The driver enforces strict state transitions to prevent hardware corruption.

## States
- `UNINITIALIZED`: The DMA Engine has not been probed.
- `ALLOCATED`: Physical memory for the BDL and Buffer has been reserved.
- `PREPARED`: The BDL has been populated with chunk metadata, and `PO_BDBAR` is loaded.
- `READY`: The hardware has verified the BDBAR mapping and is ready to fire.
- `RUNNING`: (Future Phase) The DMA `Start` bit is enabled.
- `ERROR`: A catastrophic failure occurred (e.g. Memory allocation failure, Register mismatch).

## Transition Rules
- You cannot `PREPARE` until the engine is `ALLOCATED`.
- You cannot `RUN` unless the engine is `READY`.
- A `SHUTDOWN` or `RESET` immediately returns the engine to `UNINITIALIZED`, aggressively scrubbing the RAM.
