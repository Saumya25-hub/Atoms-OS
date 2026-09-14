# ATOMS OS — M1 AUDIO FORENSIC REPORT
**Document ID**: `docs/media/M1_AUDIO_FORENSIC_REPORT.md`  
**Subsystem**: Universal Audio Hardware Foundation & BOS Audio HAL Upgrade (M1)  
**Task Phase**: TASK 1 — FORENSIC REPORT (NO CODE MODIFICATIONS)  
**Date**: September 10, 2026  

---

## 1. Forensic Evidence & Root Cause Analysis

### 1.1 Physical Hardware Incompatibility Root Cause
- **Evidence**: On the physical target machine (Intel Haswell H81 motherboard, LGA1150), the integrated audio device is an **Intel Lynx Point High Definition Audio Controller** (`PCI 0x8086:0x8C20`, Class `0x04`, Subclass `0x03`).
- **Location**: `kernel/audio/hal/audio_driver_registry.c` line 30 queries `pci_find_by_class(0x04, 0x01)` or `pci_find_by_class(0x04, 0x03)`. It discovers the controller.
- **Root Cause**: The only registered driver is `ac97_driver` (`kernel/audio/drivers/ac97/ac97.c`). In `ac97_hal_init()` (lines 18–24), the code checks:
  ```c
  uint32_t bar0 = pci_read_config(target_bus, target_slot, 0, 0x10);
  uint32_t bar1 = pci_read_config(target_bus, target_slot, 0, 0x14);
  if (!(bar0 & 1) || !(bar1 & 1)) {
      display_print("[AC97] FAILED: BARs are not I/O mapped.\n");
      return false;
  }
  ```
  Intel HDA controllers use 64-bit Memory-Mapped I/O (MMIO), where bit 0 is 0. The AC97 driver aborts initialization, leaving physical hardware completely silent.

### 1.2 Userspace Audio Disconnection Root Cause
- **Evidence**: `userspace/libs/audio/audio_user.c` contains only empty stubs. `audio_stream_write()` returns `packet->frame_count` and discards all PCM data.
- **Root Cause**: `userspace/runtime/c/include/atoms_syscall.h` and `kernel/core/syscall/src/dispatcher.c` contain zero audio system calls. There is no ABI path connecting userspace applications to the kernel audio core.

### 1.3 Third-Party Licensing Root Cause
- Linux kernel HDA implementations are GPL-2.0-or-later and cannot be imported without violating ATOMS licensing boundaries.
- FreeBSD `snd_hda` and NetBSD `azalia` are licensed under permissive **BSD-2-Clause** and **BSD-3-Clause**, providing the required register and verb definitions safely.

---

## 2. Files Involved

### 2.1 External Files to Ingest (Permissive BSD-2-Clause)
- `third_party/audio/intel_hda/LICENSE`
- `third_party/audio/intel_hda/NOTICE`
- `third_party/audio/intel_hda/README.BOS`
- `third_party/audio/intel_hda/include/hda_reg.h`
- `third_party/audio/intel_hda/include/hda_codec.h`

### 2.2 Kernel Driver Files to Create
- `kernel/audio/drivers/hda/bos_hda.h`
- `kernel/audio/drivers/hda/bos_hda_controller.c`
- `kernel/audio/drivers/hda/bos_hda_codec.c`
- `kernel/audio/drivers/hda/bos_hda_adapter.c`

### 2.3 Kernel Audio Files to Modify
- `kernel/audio/hal/audio_driver_registry.c` (Register HDA driver alongside AC97 driver, dispatch by PCI subclass)
- `kernel/audio/hal/audio_hal.c` (Support dynamic backend selection)
- `kernel/core/syscall/include/syscall.h` (Add `SYS_AUDIO_CALL 43U` and `ATOMS_AUDIO_OP_*`)
- `kernel/core/syscall/src/dispatcher.c` (Handle `SYS_AUDIO_CALL`)

### 2.4 Userspace Files to Modify
- `userspace/runtime/c/include/atoms_syscall.h` (Expose `SYS_AUDIO_CALL`)
- `userspace/libs/audio/audio_user.c` (Wire `audio_stream_*` to `SYS_AUDIO_CALL`)

### 2.5 Build & Verification Files
- `build.ps1` (Compile HDA driver and wire into kernel image)
- `tools/verify_phase_m1_audio.py` (Automated QEMU verification with `-device intel-hda -device hda-duplex` and `-device AC97`)

---

## 3. Risk Analysis

1. **Hardware MMIO Mapping Risk**: Mapping BAR0 requires allocating page table entries in the active PML4. Mitigation: Use standard `vmm_map_page()` with `PAGE_CACHE_DISABLE` as proven in `xhci.c`.
2. **Codec Initialization Timeout Risk**: Some codecs take up to 100 ms to leave reset. Mitigation: Implement bounded polling with timeout loops.
3. **Interrupt Storm Risk**: Level-triggered PCI interrupts can storm if not cleared. Mitigation: Clear status bits (`SD_STS`) immediately upon completion, and support polled DMA mode like AC97.
4. **License Contamination Risk**: Ingesting GPL code would breach licensing. Mitigation: Strictly import only BSD-2-Clause / BSD-3-Clause headers into `third_party/audio/intel_hda/`.

---

## 4. Suspected Fix (High-Level Architecture — No Code)

1. Import FreeBSD BSD-2-Clause HDA register & verb headers into `third_party/audio/intel_hda/`.
2. Construct native `bos_hda_controller.c` (MMIO mapping, CRST reset, CORB/RIRB, DMA streams).
3. Construct native `bos_hda_codec.c` (AFG discovery, Realtek ALC detection, DAC-to-Pin routing, unmuting).
4. Construct native `bos_hda_adapter.c` implementing `audio_hal_driver_t`.
5. Update `audio_driver_registry.c` to bind `0x04/0x01` to AC97 and `0x04/0x03` to HDA.
6. Allocate `SYS_AUDIO_CALL (43U)` and implement dispatcher.
7. Wire `userspace/libs/audio/audio_user.c` to real syscalls.
