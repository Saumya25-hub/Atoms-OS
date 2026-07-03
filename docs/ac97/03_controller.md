# AC'97 Driver Foundation - Controller

The AC'97 architecture is split into two halves: the Controller and the Codec.

## The Controller (Bus Master)
The Controller (managed via the `NABM` Base Address Register) lives on the PCI bus and handles memory transfers (DMA) between system RAM and the audio hardware.

In Phase 7.5, our interaction with the controller is limited to **Global Status and Control**:
- `AC97_NABM_GLOB_CNT (0x2C)`: Used to trigger hardware resets.
- `AC97_NABM_GLOB_STA (0x30)`: Queried to verify that the Codec has finished waking up.

In the upcoming playback phases, the Controller will be programmed with Buffer Descriptor Lists (BDLs) to stream PCM data without CPU intervention.
