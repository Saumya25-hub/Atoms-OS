# AC'97 Driver Foundation - Register Map

The `ac97_registers.h` file acts as the single source of truth for hardware memory offsets.

## NAM (Mixer) Registers
- `0x00` Reset / Capabilities
- `0x02` Master Volume (Controls final analog output level)
- `0x18` PCM Out Volume (Controls the digital stream volume)
- `0x26` Powerdown Control/Status
- `0x2C` PCM Front DAC Rate (Used for setting 44.1kHz / 48kHz)

## NABM (Bus Master) Registers
- `0x10` PO_BDBAR (Playback Descriptor Base Address)
- `0x14` PO_CIV (Current Index Value)
- `0x15` PO_LVI (Last Valid Index)
- `0x1B` PO_CR (Control Register - Start/Stop DMA)
- `0x2C` Global Control (Used for Resets)
- `0x30` Global Status (Used for Codec Ready detection)
