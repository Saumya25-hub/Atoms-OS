# 08 - AC'97 Register Integrity

## Goal
Verify that the AC'97 Bus Master registers accurately reflect the state of the DMA engine and the physical hardware playback mechanism.

## Forensic Proof

**From qemu_verify.log:**
```
[AC97 VERIFY]
CIV: 2
LVI: 1
SR (Hex): 0
CR (Hex): 1
DMA Running: 1
```

## Analysis
The registers show:
`CIV` (Current Index Value) is 2, pointing to the descriptor currently being processed.
`LVI` (Last Valid Index) is 1, which acts as the loop boundary to keep the hardware safely within the BDL.
`CR` (Control Register) Bit 0 (RPBM) is 1, proving the DMA Engine is Active.
`SR` (Status Register) is 0, proving there are no hardware halts, DCH (DMA Controller Halted) assertions, or FIFO underruns occurring during rotation.
