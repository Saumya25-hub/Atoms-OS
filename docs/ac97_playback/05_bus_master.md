# AC'97 Playback - Bus Master Control

The Bus Master is the autonomous DMA controller living on the PCI bus. It is controlled via the NABM Base Address.

## Control Registers
- **PO_CR (Control Register)**: By asserting the RPBM (Run/Pause) bit `0x01`, the Bus Master begins iterating through the BDL.
- **PO_CIV (Current Index Value)**: Read-only register indicating which descriptor the Bus Master is currently transmitting to the Codec.
- **PO_LVI (Last Valid Index)**: Read-write register denoting the final descriptor the software has prepared. Once `CIV == LVI`, the Bus Master halts to prevent under-running.
- **PO_SR (Status Register)**: Allows software to detect if the Bus Master has successfully halted (`DCH` bit) after a stop command or a buffer underrun.

## Stop Sequence
To safely stop the Bus Master, the software clears the RPBM bit and then enters a tight polling loop on `PO_SR` waiting for the `DCH` (Halt) bit to assert. This ensures the hardware is physically dormant before any memory is freed.
