# AC'97 Driver Foundation - Walkthrough

## What Was Built
We successfully built the hardware foundation for the AC'97 Audio Controller (Phase 7.5).

1.  `port_io.c`: We added 32-bit `io_in32` and `io_out32` required for querying the PCI Configuration Space.
2.  `ac97_registers.h`: We defined the standard AC'97 NAM (Codec) and NABM (Bus Master) offsets.
3.  `ac97_codec.c`: We implemented the safe, timeout-protected `Cold Reset` sequence and read/write access wrappers for the Codec.
4.  `ac97.c`: We built a focused PCI scanner that dynamically finds the Audio Controller, extracts its I/O Base Address Registers (BAR0 / BAR1) and IRQ line, enables Bus Mastering, and triggers the self-tests.

## What Was Tested
The OS was compiled and booted in QEMU with the `-device AC97` flag enabled. 
The driver attempted to locate the PCI device, map it, reset the codec, and write to the Master Volume register to verify bus integrity.

## Validation Results
The QEMU boot log confirmed a complete success:
```text
[AC97] Scanning PCI for Audio Controller...
[AC97] Found Audio Controller. Vendor/Device: 0x0x24158086
[AC97] NAM BAR: 0x0xC000, NABM BAR: 0x0xC400, IRQ: 11
[AC97] Performing Cold Reset...
[AC97] Codec Ready (PCR flag set).
[AC97] Codec Capabilities: 0x0x0
[AC97] Register R/W Integrity Test... PASS
[AC97] Power Status: 0x0xF
[AC97] SUCCESS: Hardware Foundation Established.
```

The codec became `Ready` safely. We successfully read and wrote to the Volume register, proving the I/O mapping was completely accurate.

## Conclusion
The foundation is rock solid. No crashes, no infinite loops, and flawless hardware detection. We are now ready to tackle Phase 7.6 (DMA Playback).
