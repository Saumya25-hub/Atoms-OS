# AC'97 DMA Engine - Hardware Registers

The Bus Master controller is programmed via the NABM Base Address.

## Core Playback Registers
- `PO_BDBAR (0x10)`: The Base Address of the 32-entry BDL array in physical RAM.
- `PO_LVI (0x15)`: Last Valid Index (0-31). Tells the controller how many descriptors in the list are valid and ready to be played.
- `PO_CIV (0x14)`: Current Index Value. A read-only register that tells the software which descriptor the hardware is currently playing.
- `PO_CR (0x1B)`: Control Register. Used to Start (0x01) and Reset (0x02) the DMA channel.
- `PO_SR (0x16)`: Status Register. Used to check if the DMA channel has successfully halted.

## The Preparation Sequence
1. The channel is halted by writing `0x00` to `PO_CR`.
2. Software polls `PO_SR` waiting for the `DCH` (DMA Controller Halted) bit.
3. The channel is fully reset via `0x02`.
4. The Physical Address of the BDL array is written to `PO_BDBAR`.
5. The `LVI` is set to `31`.
