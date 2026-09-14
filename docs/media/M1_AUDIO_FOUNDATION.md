# ATOMS OS — M1 AUDIO HARDWARE FOUNDATION GUIDE
**Document ID**: `docs/media/M1_AUDIO_FOUNDATION.md`  
**Subsystem**: Universal Audio Hardware Foundation (M1)  
**Date**: September 10, 2026  
**Status**: SPECIFICATION & IMPLEMENTATION GUIDE  

---

## 1. Overview & Core Philosophy

The **M1 Universal Audio Hardware Foundation** transforms the ATOMS OS audio subsystem from a legacy, QEMU-only AC97 prototype into a modern, production-grade audio engine capable of driving real Intel High Definition Audio (HDA) controllers and codecs across physical motherboards (Haswell H81 and beyond).

### Core Design Rules:
1. **Separation of Concerns**:
   - **External Technology**: Mature, open-source HDA register definitions and protocol specifications licensed under permissive BSD-2-Clause are cleanly isolated in `third_party/audio/intel_hda/`.
   - **BOS Ownership**: Controller management, codec widget graph traversal, DMA buffering, real-time mixing, syscall routing, and application APIs remain 100% native ATOMS/BOS code.
2. **Universal Extensibility**:
   - The driver architecture does not assume H81 or any single motherboard.
   - Any PCI device declaring Class `0x04` (Multimedia) and Subclass `0x03` (HDA) is enumerated, MMIO mapped, and probed.
   - Any HDA-compliant codec (Realtek ALC887, ALC892, ALC662, ALC283, ALC269, Conexant, Analog Devices, or QEMU HDA) can be discovered and initialized through widget discovery.
3. **Coexistence**:
   - Legacy AC97 controllers (`Class 0x04, Subclass 0x01`) remain supported for backward compatibility and standard QEMU emulation.

---

## 2. Component Ownership & Boundary

| Component | Directory / File | Ownership | Origin / License | Role |
|---|---|---|---|---|
| **HDA Register Definitions** | `third_party/audio/intel_hda/include/hda_reg.h` | Third-Party | FreeBSD / BSD-2-Clause | HDA controller MMIO register offsets, bitfields, verb constants |
| **HDA Codec Definitions** | `third_party/audio/intel_hda/include/hda_codec.h` | Third-Party | FreeBSD / BSD-2-Clause | Codec widget parameters, pin capabilities, amplifier gain constants |
| **BOS HDA Driver Engine** | `kernel/audio/drivers/hda/bos_hda_controller.c` | BOS Native | ATOMS Project | PCI MMIO mapping, CRST reset, CORB/RIRB ring buffers, DMA stream BDL |
| **BOS Codec Engine** | `kernel/audio/drivers/hda/bos_hda_codec.c` | BOS Native | ATOMS Project | AFG parsing, widget discovery, Realtek ALC detection, Pin unmute & power |
| **BOS HDA HAL Adapter** | `kernel/audio/drivers/hda/bos_hda_adapter.c` | BOS Native | ATOMS Project | Implements `audio_hal_driver_t` interface for HDA |
| **Driver Registry** | `kernel/audio/hal/audio_driver_registry.c` | BOS Native | ATOMS Project | Dispatches `0x04/0x01` to AC97 and `0x04/0x03` to HDA |
| **BOS Audio HAL** | `kernel/audio/hal/audio_hal.c` | BOS Native | ATOMS Project | Uniform hardware abstraction layer |
| **BOS Audio Core** | `kernel/audio/core/audio_core.c` | BOS Native | ATOMS Project | Stream lifecycle and client process management |
| **BOS Audio Mixer** | `kernel/audio/mixer/audio_mixer.c` | BOS Native | ATOMS Project | Multi-stream summing and master volume control |
| **Syscall Dispatcher** | `kernel/core/syscall/src/dispatcher.c` | BOS Native | ATOMS Project | Dispatches `SYS_AUDIO_CALL (43U)` |
| **Userspace Library** | `userspace/libs/audio/audio_user.c` | BOS Native | ATOMS Project | Userspace C/C++ audio client API |

---

## 3. Diagnostic Telemetry Standard

During boot and hardware initialization, the driver generates explicit, forensic telemetry via `display_print()` and serial logs:

```text
[BOS-AUDIO] Audio subsystem initialization started
[BOS-AUDIO] Scanning PCI bus for multimedia controllers...
[BOS-AUDIO] Intel HDA Controller detected at PCI 00:1B.0 (Vendor: 0x8086, Device: 0x8C20)
[BOS-AUDIO] Reading BAR0: 0xF7D30004 (64-bit Memory-Mapped I/O)
[BOS-AUDIO] Mapping 16 KB MMIO pages into kernel address space: OK
[BOS-AUDIO] Enabling PCI Bus Mastering and Memory Space: OK
[BOS-AUDIO] Controller Reset (CRST): SUCCESS (Revision 1.0)
[BOS-AUDIO] Initializing CORB (1024 bytes) & RIRB (2048 bytes): READY
[BOS-AUDIO] Scanning Codec Status (STATESTS: 0x0001)...
[BOS-AUDIO] Codec #0 discovered at Address 0
[BOS-AUDIO] Querying Codec Vendor/Device ID: 0x10EC0887 (Realtek ALC887)
[BOS-AUDIO] Traversing Audio Function Group widgets (Nodes 0x02 to 0x1E)...
[BOS-AUDIO] Found DAC Widget 0x02 (Format: 48kHz, 16-bit, 2 channels)
[BOS-AUDIO] Found Pin Complex Widget 0x14 (Green Line-Out / Headphone)
[BOS-AUDIO] Connecting DAC 0x02 -> Pin 0x14: OK
[BOS-AUDIO] Unmuting Output Amplifier & Setting Power State D0: OK
[BOS-AUDIO] Initializing DMA Output Stream #1 (64 KB Cyclic Buffer, 4 BDL Chunks): OK
[BOS-AUDIO] Audio Hardware Output READY
```

If hardware initialization fails at any stage, an unambiguous `[ERROR]` is emitted, returning `false` rather than faking readiness.
