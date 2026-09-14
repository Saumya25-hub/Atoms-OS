# ATOMS OS — M1 AUDIO SUBSYSTEM ARCHITECTURE
**Document ID**: `docs/media/M1_AUDIO_ARCHITECTURE.md`  
**Subsystem**: Universal Audio Hardware Foundation & BOS Audio HAL Upgrade (M1)  
**Date**: September 10, 2026  
**Status**: ARCHITECTURE SPECIFICATION  

---

## 1. Architectural Model & Layered Separation

In accordance with Section 0 and Section 5 of the M1 directive:
- ATOMS OS is **not** Linux, Windows, or BSD.
- Third-party open-source driver technology is cleanly isolated in `third_party/audio/intel_hda/`.
- BOS remains the single owner of the Audio HAL, Mixer, PCM streams, Syscall Gateway, and Userspace API.

```
┌────────────────────────────────────────────────────────────────────────┐
│                        USERSPACE APPLICATION                           │
│  e.g., Future BOS Media Player, Games, Browser, System Sound Alerts   │
└──────────────────────────────────┬─────────────────────────────────────┘
                                   │  C / C++ API (bos_audio_*)
                                   ▼
┌────────────────────────────────────────────────────────────────────────┐
│                   USERSPACE AUDIO LIBRARY (libbos_audio)               │
│                   userspace/libs/audio/audio_user.c                    │
│   Creates stream, buffers PCM, controls playback state via syscalls    │
└──────────────────────────────────┬─────────────────────────────────────┘
                                   │  SYSCALL: SYS_AUDIO_CALL (43U)
                                   ▼
══════════════════════════════════════════════════════════════════════════
               ATOMS KERNEL PROTECTED SYSCALL GATEWAY
   kernel/core/syscall/src/dispatcher.c & include/syscall.h
   Dispatches: OP_DEVICE_ENUM, OP_STREAM_CREATE, OP_STREAM_WRITE, etc.
══════════════════════════════════════════════════════════════════════════
                                   │
                                   ▼
┌────────────────────────────────────────────────────────────────────────┐
│                        BOS AUDIO SERVICE & CORE                        │
│                   kernel/audio/core/audio_core.c                       │
│    Stream lifecycle, process isolation, client buffer queues           │
└──────────────────────────────────┬─────────────────────────────────────┘
                                   │
                                   ▼
┌────────────────────────────────────────────────────────────────────────┐
│                        BOS REAL-TIME AUDIO MIXER                       │
│                   kernel/audio/mixer/audio_mixer.c                     │
│    Multi-stream summing, master volume attenuation, PCM conversion    │
└──────────────────────────────────┬─────────────────────────────────────┘
                                   │
                                   ▼
┌────────────────────────────────────────────────────────────────────────┐
│                          BOS AUDIO HAL                                 │
│                   kernel/audio/hal/audio_hal.c                         │
│    Uniform hardware abstraction (start, stop, pause, resume, update)   │
└──────────────────┬──────────────────────────────────┬──────────────────┘
                   │                                  │
      PCI Class 04/01 (I/O BAR)          PCI Class 04/03 (MMIO BAR)
                   ▼                                  ▼
┌──────────────────────────────┐   ┌─────────────────────────────────────┐
│       LEGACY AC97 DRIVER     │   │        BOS HDA ADAPTER LAYER        │
│  kernel/audio/drivers/ac97/  │   │     kernel/audio/drivers/hda/       │
│  QEMU AC97 Emulation         │   │     bos_hda_adapter.c               │
└──────────────────────────────┘   └──────────────────┬──────────────────┘
                                                      │
                                                      ▼
                                   ┌─────────────────────────────────────┐
                                   │     BOS HDA CONTROLLER & CODEC      │
                                   │     bos_hda_controller.c            │
                                   │     bos_hda_codec.c                 │
                                   │     • CORB/RIRB Ring Buffers        │
                                   │     • DMA Stream Engine & BDL       │
                                   │     • Codec Parsing & Realtek Setup │
                                   └──────────────────┬──────────────────┘
                                                      │
                                                      ▼
                                   ┌─────────────────────────────────────┐
                                   │     THIRD-PARTY HDA REGISTER SPECS  │
                                   │     third_party/audio/intel_hda/    │
                                   │     (FreeBSD BSD-2-Clause hda_reg)  │
                                   └──────────────────┬──────────────────┘
                                                      │
                                                      ▼
                                   ┌─────────────────────────────────────┐
                                   │        PHYSICAL HARDWARE            │
                                   │  • Intel Lynx Point HDA (H81)       │
                                   │  • Realtek ALC Codec (ALC887/ALC283)│
                                   │  • QEMU Intel HDA (-device intel-hda│
                                   └─────────────────────────────────────┘
```

---

## 2. Directory Structure & Source Separation

```
d:\Signatures_OS\
├── third_party\
│   └── audio\
│       └── intel_hda\
│           ├── LICENSE             (BSD-2-Clause license text)
│           ├── NOTICE              (FreeBSD copyright notices)
│           ├── README.BOS          (Provenance, modifications, role)
│           └── include\
│               ├── hda_reg.h       (HDA register & verb definitions)
│               └── hda_codec.h     (Codec widget parameter flags)
│
├── kernel\
│   ├── audio\
│   │   ├── drivers\
│   │   │   ├── ac97\               (Preserved legacy AC97 driver)
│   │   │   └── hda\                [NEW] BOS-native HDA driver engine
│   │   │       ├── bos_hda.h       (Controller & codec structs)
│   │   │       ├── bos_hda_controller.c (MMIO, CRST reset, CORB/RIRB, DMA)
│   │   │       ├── bos_hda_codec.c (Widget discovery, Realtek ALC, Pins)
│   │   │       └── bos_hda_adapter.c (BOS Audio HAL bridge)
│   │   └── hal\
│   │       ├── audio_driver_registry.c (Updated PCI class detection)
│   │       └── audio_hal.c         (BOS HAL driver dispatch)
│   │
│   └── core\
│       └── syscall\
│           ├── include\
│           │   └── syscall.h       (Added SYS_AUDIO_CALL 43U + sub-ops)
│           └── src\
│               └── dispatcher.c    (Added case SYS_AUDIO_CALL handling)
│
└── userspace\
    ├── libs\
    │   └── audio\
    │       └── audio_user.c        (Wired to SYS_AUDIO_CALL syscalls)
    └── runtime\
        └── c\
            └── include\
                └── atoms_syscall.h (Exposed SYS_AUDIO_CALL to userspace)
```

---

## 3. Hardware Discovery & Registry Flow

### 3.1 Resolving the Existing AC97 Detection Failure
Previously, `audio_driver_registry.c` accepted both `0x04, 0x01` and `0x04, 0x03` but only had `ac97_driver` registered, causing physical Intel HDA controllers to fail on AC97's I/O BAR assertion.

Under the new architecture:
1. `ac97_driver` explicitly checks for PCI Class `0x04`, Subclass `0x01` (or device matches like `0x8086:0x2415`). If BAR0/BAR1 are not I/O mapped, it rejects the device cleanly without halting system init.
2. `hda_driver` registers with the registry:
   - Probes PCI devices matching Class `0x04` (Multimedia), Subclass `0x03` (High Definition Audio).
   - Reads 64-bit MMIO BAR0 (`0x10` and `0x14`).
   - Enables PCI Bus Mastering (`pci_enable_bus_mastering`) and Memory Space (`pci_enable_memory_space`).
   - Maps 16 KB of MMIO pages into kernel virtual space using `vmm_map_page()`.
3. When booted in QEMU with `-device AC97`: AC97 driver initializes.
4. When booted in QEMU with `-device intel-hda`: HDA driver initializes.
5. When booted on physical Haswell H81 hardware: HDA driver initializes.

---

## 4. Intel HDA Controller Engine (`bos_hda_controller.c`)

### 4.1 Global Initialization Sequence
1. **Controller Reset (CRST)**:
   - Read `GCTL` (offset `0x08`).
   - Clear bit 0 (`CRST = 0`) to enter reset state; wait for hardware to acknowledge `CRST == 0`.
   - Write bit 0 (`CRST = 1`) to exit reset; wait for hardware to acknowledge `CRST == 1`.
2. **Interrupts**:
   - Write `INTCTL` (offset `0x20`) to enable Global Interrupts (`GIE = 1`).
3. **Codec Discovery**:
   - Read `STATESTS` (offset `0x0E`) to detect which codec address bits (0..14) are asserted.
   - Typically, Primary Codec is at address `0` (`STATESTS & 0x01`).

### 4.2 Command Output Ring Buffer (CORB)
- Pre-allocated 1024-byte DMA buffer (256 entries $\times$ 4 bytes), 128-byte aligned.
- Physical address programmed into `CORBLBASE` (`0x40`) and `CORBUBASE` (`0x44`).
- Set `CORBSIZE` (`0x4E`) to 256 entries (`0x02`).
- Reset read pointer `CORBRP` (`0x4A`, bit 15), then start CORB run bit (`CORBCTL`, bit 1).

### 4.3 Response Input Ring Buffer (RIRB)
- Pre-allocated 2048-byte DMA buffer (256 entries $\times$ 8 bytes), 128-byte aligned.
- Physical address programmed into `RIRBLBASE` (`0x50`) and `RIRBUBASE` (`0x54`).
- Set `RIRBSIZE` (`0x5E`) to 256 entries (`0x02`).
- Start RIRB run bit (`RIRBCTL`, bit 1).

### 4.4 Immediate Command Mode (IC/IR Fallback)
- For maximum reliability across early boot and diverse BIOS firmware, the controller engine provides dual-mode verb submission:
  - High-speed CORB/RIRB ring mode.
  - Immediate Command (`ICS` / `IC` / `IR`) mode: Writes 32-bit verb to `0x60`, asserts `ICS` busy, polls for response in `0x64`.

### 4.5 DMA Output Stream Engine
- Locates the first output stream descriptor (offset `0x80 + (num_iss * 0x20)`).
- Allocates a contiguous DMA buffer (e.g. 64 KB = 16,384 frames at 48kHz 16-bit stereo).
- Configures Buffer Descriptor List (BDL) with 4 entries $\times$ 16 KB chunks.
- Sets stream format (`SD_FMT`): 48 kHz, 16-bit, 2 channels (`0x0011`).
- Sets stream tag (`stream_tag = 1`).

---

## 5. Codec Discovery & Realtek Engine (`bos_hda_codec.c`)

### 5.1 Discovery & Parameter Extraction
1. Query Node `0` for Subordinate Node Count (`Verb 0xF0004`): Discovers root node range.
2. Query Audio Function Group (AFG): Discovers widget range (typically nodes `0x02` through `0x20`).
3. Query Vendor & Device ID (`Verb 0xF0000`):
   - `0x10EC0887` -> Realtek ALC887
   - `0x10EC0892` -> Realtek ALC892
   - `0x10EC0662` -> Realtek ALC662
   - `0x10EC0283` -> Realtek ALC283
   - `0x10EC0269` -> Realtek ALC269
   - `0x10ECxxxx` -> Generic Realtek ALC Codec
   - Other -> Generic High Definition Audio Codec

### 5.2 Widget Graph Traversal & Output Path Construction
1. **DAC / Output Converter Search**:
   - Scans nodes where Widget Type == `0x0` (Audio Output).
   - Verifies 48 kHz 16-bit PCM support.
2. **Pin Complex Search**:
   - Scans nodes where Widget Type == `0x4` (Pin Complex).
   - Checks Pin Capabilities: Must have Output Capable (`bit 4`) or Headphone Drive (`bit 2`).
3. **Path Association**:
   - Configures connection select (`Verb 0x705`) to route the DAC output to the Pin Complex.
   - Sets Pin Widget Control (`Verb 0x707`): Asserts `0x40` (Output Enable) and `0xC0` (Headphone Enable).
   - Sets Power State (`Verb 0x701`): `0x00` (Fully Active D0).
   - Unmutes Amplifier Gain (`Verb 0x3A0` / `0x3B0`): Sets gain to 0 dB, mute bit = 0.
   - Sets Stream Channel (`Verb 0x706`): `(stream_tag << 4) | 0`.

---

## 6. Userspace Syscall Gateway Interface

### 6.1 Syscall Allocation
In `kernel/core/syscall/include/syscall.h`:
```c
#define SYS_AUDIO_CALL 43U
#define MAX_SYSCALL    44U

/* Audio Syscall Operations */
#define ATOMS_AUDIO_OP_DEVICE_GET_INFO 1U
#define ATOMS_AUDIO_OP_STREAM_CREATE   2U
#define ATOMS_AUDIO_OP_STREAM_DESTROY  3U
#define ATOMS_AUDIO_OP_STREAM_WRITE    4U
#define ATOMS_AUDIO_OP_STREAM_START    5U
#define ATOMS_AUDIO_OP_STREAM_STOP     6U
#define ATOMS_AUDIO_OP_STREAM_PAUSE    7U
#define ATOMS_AUDIO_OP_STREAM_RESUME   8U
#define ATOMS_AUDIO_OP_STREAM_GET_POS  9U
#define ATOMS_AUDIO_OP_DEVICE_SET_VOL  10U
```

### 6.2 Data Transport ABI
- `audio_user.c` calls `__atoms_syscall3(SYS_AUDIO_CALL, ATOMS_AUDIO_OP_STREAM_CREATE, ...)` to register a stream with `audio_core`.
- In `audio_stream_write(stream_id, packet)`:
  - Invokes `__atoms_syscall4(SYS_AUDIO_CALL, ATOMS_AUDIO_OP_STREAM_WRITE, stream_id, packet_ptr)`.
  - Kernel validates user memory bounds via `syscall_validate_user_buffer()`, pulls PCM frames into the stream ring buffer, and schedules the mixer.
