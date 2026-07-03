# AC'97 Driver Foundation - Codec

The Codec is the physical analog-to-digital (ADC) and digital-to-analog (DAC) converter. 

## Communication
The Codec is controlled via the `NAM` Base Address Register. Unlike the Controller which handles DMA, the Codec handles volume, power states, and sample rate configurations.

## Resets
The Codec is notoriously slow to wake up. `ac97_codec.c` implements two reset mechanisms:
1. **Cold Reset**: Hard-toggles the AC-Link via the Controller's `GLOB_CNT` register. This physically reboots the Codec. It requires significant delay loops for the signal to propagate.
2. **Warm Reset**: Used to wake the codec from a low-power suspended state without losing register configurations.

After any reset, the driver must enter a timeout loop querying the `PCR` (Primary Codec Ready) bit in the Controller's status register before attempting to read or write any volume registers.
