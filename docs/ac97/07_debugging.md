# AC'97 Driver Foundation - Debugging & Risks

## Known Risks

### Infinite Loops on Missing Hardware
Real hardware does not always conform perfectly to the AC'97 specification. 
If a codec is damaged or improperly seated, reading the `PCR` (Codec Ready) bit might return `0` forever. 
To prevent a complete kernel freeze, `ac97_codec_wait_ready()` implements a strict upper-bound timeout loop using `500,000` iterations. If the timeout expires, the function returns `false`, allowing the kernel to gracefully disable audio and continue booting.

### I/O Port Conflicts
If another driver incorrectly brute-forces I/O ports without parsing PCI BARs, it could accidentally write to the AC'97 controller's memory space, triggering unintended DMA streams. The AC'97 driver strictly relies on the PCI Configuration Space to dynamically acquire its I/O boundaries at boot.

## Debug Points
The telemetry log prints:
`[AC97] Codec Capabilities: 0xXXXX`
`[AC97] Power Status: 0xXXXX`

These registers are critical for diagnosing faulty hardware. If the capabilities register returns `0xFFFF`, the PCI I/O mapping was likely stripped or blocked by a hypervisor.
