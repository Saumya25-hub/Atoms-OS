# ATOMS OS — M1 AUDIO PATCH PLAN
**Document ID**: `docs/media/M1_AUDIO_PATCH_PLAN.md`  
**Subsystem**: Universal Audio Hardware Foundation & BOS Audio HAL Upgrade (M1)  
**Task Phase**: TASK 2 — ARCHITECT TEAM (NO CODE MODIFICATIONS)  
**Input**: `docs/media/M1_AUDIO_FORENSIC_REPORT.md`  
**Date**: September 10, 2026  

---

## 1. Summary of Planned Modifications

### Phase 1: Ingest Permissive External HDA Specification
- **Target Directory**: `third_party/audio/intel_hda/`
- **Files**:
  - `LICENSE`: Full BSD-2-Clause legal text.
  - `NOTICE`: Attribution for Stephane E. Fabie, Alexander Motin, and The FreeBSD Project.
  - `README.BOS`: Explaining upstream version, license, and modifications.
  - `include/hda_reg.h`: Hardware register offsets, CORB/RIRB bitfields, HDA verb definitions.
  - `include/hda_codec.h`: Codec parameters, pin widget controls, amplifier gain constants.
- **Why**: Avoid reinventing standard Intel HDA register definitions and verb encodings.
- **Expected Result**: Clean, freestanding, BSD-2-Clause specification headers available for kernel compilation.

### Phase 2: Implement BOS-Native HDA Controller & Codec Layer
- **Target Directory**: `kernel/audio/drivers/hda/`
- **Files**:
  - `bos_hda.h`: Unified driver structures (`bos_hda_controller_t`, `bos_hda_codec_t`, `bos_hda_stream_t`).
  - `bos_hda_controller.c`: PCI discovery, 64-bit MMIO BAR0 mapping (`vmm_map_page`), controller reset (`CRST`), CORB/RIRB ring initialization, DMA stream buffer allocation (contiguous physical pages via `vmm_get_physical_address`), BDL configuration.
  - `bos_hda_codec.c`: Codec detection via `STATESTS`, vendor identification (`0x10EC` for Realtek ALC), Audio Function Group discovery, DAC audio output widget search, Pin complex search, connection select routing, power state D0 setting, amplifier unmuting (0 dB gain).
  - `bos_hda_adapter.c`: Implements `audio_hal_driver_t` vtable (`init`, `shutdown`, `start_stream`, `stop_stream`, `pause_stream`, `resume_stream`, `update_pointers`), binding HDA directly into the existing BOS Audio HAL.
- **Why**: Drive real Intel HDA hardware on physical H81 motherboards and in QEMU.
- **Expected Result**: Physical audio output through 3.5mm headphone/line-out jack.

### Phase 3: Update Driver Registry & HAL Driver Selection
- **Target File**: `kernel/audio/hal/audio_driver_registry.c`
- **Modification**:
  - Register both `ac97_driver` and `hda_driver`.
  - In `audio_driver_registry_discover_active()`:
    - If device subclass is `0x01` (AC97): Select `ac97_driver`.
    - If device subclass is `0x03` (HDA): Select `hda_driver`.
- **Target File**: `kernel/audio/drivers/ac97/ac97.c`
- **Modification**:
  - Prevent AC97 from attempting to initialize if BARs are not I/O mapped, returning `false` gracefully without error logs on HDA hardware.
- **Why**: Fix the current AC97 initialization failure on physical hardware and allow seamless coexistence between QEMU AC97 and physical HDA.
- **Expected Result**: Clean driver selection based on actual PCI device architecture.

### Phase 4: Userspace Syscall Gateway & Library Wiring
- **Target Files**:
  - `kernel/core/syscall/include/syscall.h`: Define `SYS_AUDIO_CALL (43U)` and sub-operations `ATOMS_AUDIO_OP_*`. Increment `MAX_SYSCALL` to `44U`.
  - `kernel/core/syscall/src/dispatcher.c`: Implement `case SYS_AUDIO_CALL:` routing to `audio_core` and `audio_mixer`.
  - `userspace/runtime/c/include/atoms_syscall.h`: Expose `SYS_AUDIO_CALL (43U)` to userspace C runtime.
  - `userspace/libs/audio/audio_user.c`: Replace dummy no-op stubs with real syscall invocations (`__atoms_syscall*`).
- **Why**: Connect userspace C/C++ applications to the kernel audio engine.
- **Expected Result**: Userspace applications can open audio streams, write PCM buffers, and control playback.

### Phase 5: Build Integration & Automated Verification
- **Target File**: `build.ps1`
- **Modification**:
  - Compile `kernel/audio/drivers/hda/bos_hda_controller.c`, `bos_hda_codec.c`, `bos_hda_adapter.c` into object files.
  - Include them in `link_response.txt` for `kernel.elf`.
- **Target File**: `tools/verify_phase_m1_audio.py`
- **Modification**:
  - Automated test script validating QEMU pure UEFI boot with `-device intel-hda -device hda-duplex` and checking serial logs for successful HDA discovery, codec detection, and PCM stream initialization.
- **Why**: Verify clean build and pre-flight execution in accordance with Pre-Flash Verification Rules.
- **Expected Result**: Clean build with zero warnings; pre-flight PASS in QEMU.

---

## 2. Risk & Impact Analysis

| Component | Risk | Impact | Mitigation |
|---|---|---|---|
| **PCI MMIO Mapping** | Page fault on invalid MMIO base | High | Validate BAR0 address, mask out flags (`& ~0xF`), map with `PAGE_CACHE_DISABLE`. |
| **HDA Codec Timeout** | Hanging loop during codec response | Medium | Implement cycle/counter-bounded timeout in verb submission loops. |
| **DMA Buffer Alignment** | DMA corruption on unaligned buffers | High | Align CORB/RIRB to 128 bytes; align BDL to 128 bytes; verify physical contiguity via `vmm_get_physical_address`. |
| **Existing AC97 Regression** | Breaking legacy QEMU audio | Medium | Retain AC97 driver untouched; test QEMU with `-device AC97`. |
| **Syscall Collision** | Breaking existing userspace syscalls | High | Allocate `SYS_AUDIO_CALL` at 43U (strictly above current `MAX_SYSCALL 43U`). |

---

## 3. Rollback Plan

If regressions occur:
1. All newly added HDA files are isolated in `third_party/audio/intel_hda/` and `kernel/audio/drivers/hda/`. Deleting these directories reverts HDA.
2. Changes to `audio_driver_registry.c` and `dispatcher.c` are modular and can be reverted cleanly via git checkout.
3. `build.ps1` changes can be reverted without touching any other subsystems.
