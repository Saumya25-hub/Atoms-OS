# ATOMS OS — M1 AUDIO HARDWARE VALIDATION PROTOCOL
**Document ID**: `docs/media/M1_AUDIO_HARDWARE_VALIDATION.md`  
**Subsystem**: Universal Audio Hardware Foundation (M1)  
**Hardware Platforms**: QEMU Pure UEFI (x86_64) & Intel Haswell H81 Motherboard (Real Hardware)  
**Date**: September 10, 2026  
**Status**: VALIDATION SPECIFICATION  

---

## 1. Physical Hardware & Emulation Targets

### Target 1: Physical Bare-Metal Testbench
- **Motherboard**: H81 Motherboard (Haswell LGA1150 Chipset)
- **BIOS Firmware**: 2022 Updated BIOS (Native UEFI Mode)
- **CPU**: Intel Core i3 4th Gen (Haswell x86_64)
- **Audio Controller**: Intel Lynx Point PCH HD Audio Controller (`PCI 0x8086:0x8C20`, Class `0x04`, Subclass `0x03`)
- **Audio Codec**: Realtek ALC887 / ALC892 / ALC662 (High Definition Audio Codec)
- **RAM**: 8 GB DDR3

### Target 2: QEMU Emulation Targets
- **Target 2A (Intel HDA)**: QEMU pure UEFI with `-device intel-hda -device hda-duplex`
- **Target 2B (Legacy AC97)**: QEMU pure UEFI with `-device AC97`

---

## 2. Forensic Test Suite (Tests A through F)

| Test ID | Objective | Input / Stimulus | Expected Output | Verification Method |
|---|---|---|---|---|
| **Test A: PCI Detection** | Verify PCI bus discovery of Intel HDA controller | PCI scan during kernel boot | Detects `Class 0x04, Subclass 0x03`, reads BAR0 | Serial log: `[BOS-AUDIO] Intel HDA Controller detected` |
| **Test B: Codec Discovery** | Verify codec link and vendor discovery | Read `STATESTS`, submit Verb `0xF0000` | Codec ID read; vendor identified (Realtek `0x10EC` or QEMU `0x1AF4`/`0x8384`) | Serial log: `[BOS-AUDIO] Querying Codec Vendor/Device ID` |
| **Test C: Path Configuration** | Verify DAC to Pin routing and unmute | Verbs `0x705` (select), `0x707` (pin ctrl), `0x3A0` (unmute), `0x701` (D0) | Output pin enabled (`0xC0`), amplifier unmuted (`0 dB`), D0 power active | Verb response status check |
| **Test D: PCM Stream Setup** | Verify DMA buffer allocation and BDL setup | Configure 48 kHz, 16-bit, stereo stream | BDL entries point to valid contiguous physical DMA buffers, `SD_CTL` running | Register `SD_CTL & 0x02 == 0x02` |
| **Test E: Synthetic Audio Output** | Verify sound output with pure tone | Generate 440 Hz / 1000 Hz sine wave tone | Audible audio from Line-Out / Headphone jack | Physical earphone/speaker listening test |
| **Test F: Sustained Playback** | Verify zero memory leak, underrun, or DMA stall | Continuous playback for 60s to 300s | Continuous audio without stutter, `SD_STS` FIFO error flag is zero | LPIB pointer advance and telemetry monitor |

---

## 3. Regression Test Protocol

To guarantee zero regression across ATOMS OS:
1. **Compilation**: Clean build of `BOOTX64.EFI`, `kernel.elf`, `libbos_ui_cpp.a`, `desktop_shell.elf`, and all 5 demo ELFs.
2. **UEFI Boot**: QEMU boots in pure UEFI mode directly to desktop shell without panics.
3. **AC97 Verification**: Launching QEMU with `-device AC97` maintains functional legacy audio path.
4. **HDA Verification**: Launching QEMU with `-device intel-hda -device hda-duplex` verifies new HDA path.
5. **Desktop & UI Stability**: Mouse pointer, rounded windows, keyboard input, and BWE compositor remain 100% responsive during audio playback.
