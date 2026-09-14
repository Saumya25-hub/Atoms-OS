# ATOMS OS — Third-Party GPU & Video Acceleration License Audit
**Document ID**: `docs/media/THIRD_PARTY_GPU_LICENSE_AUDIT.md`  
**Subsystem**: Hardware Video Acceleration HAL, Vendor Driver References, Firmware Interfaces  
**Date**: September 10, 2026  
**Auditor**: ATOMS OS Compliance & Architecture Authority  
**Verdict**: **STRICT COMPLIANCE MAINTAINED (ZERO GPL CONTAGION)**  

---

## 1. Objective & Policy

The ATOMS OS core architectural mandate strictly forbids the importation of Copyleft (GPL-2.0, GPL-3.0, LGPL-3.0) licensed source code into the kernel or base operating system components.

This audit evaluates all external open-source references, vendor SDKs, and driver architectures studied for the **Universal Hardware Video Acceleration** subsystem (Intel, NVIDIA, AMD).

---

## 2. License Evaluation Matrix

| Subsystem / Reference Project | Originating Organization | Upstream URL | Declared License | ATOMS OS Usage Policy | Compatibility Status |
|---|---|---|---|---|---|
| **Intel media-driver (`iHD`)** | Intel Corporation | `github.com/intel/media-driver` | MIT License / BSD-3-Clause | Clean reference for MMIO command sequences, MFX/VDBox batch buffer structures, and tiling formats | **COMPLIANT** (Permissive) |
| **NVIDIA open-gpu-kernel-modules** | NVIDIA Corporation | `github.com/NVIDIA/open-gpu-kernel-modules` | Dual MIT / GPL-2.0 | Reference for BAR0/BAR1 MMIO mapping, GSP firmware protocol, and channel FIFO ring allocation. ATOMS OS strictly adopts the MIT licensing path | **COMPLIANT** (Permissive under MIT) |
| **NVIDIA Video Codec SDK (`nvcuvid`)** | NVIDIA Corporation | `developer.nvidia.com/video-codec-sdk` | NVIDIA SDK License (Headers MIT-compatible) | Clean-room definition of picture parameter structures (`CUVIDPICPARAMS`) and sequence headers. No proprietary binary DLLs linked into kernel | **COMPLIANT** (Clean-room) |
| **AMD rocDecode** | Advanced Micro Devices | `github.com/ROCm/rocDecode` | MIT License | Clean reference for VCN sequence parsing, Indirect Buffer (IB) packet structure, and surface formatting | **COMPLIANT** (Permissive) |
| **Mesa 3D (RadeonSI / VA-API)** | Mesa 3D Project | `gitlab.freedesktop.org/mesa/mesa` | MIT License | Architecture reference for VCN Ring 0/1 packet formatting and fence registers | **COMPLIANT** (Permissive) |
| **Linux Kernel DRM / GEM / i915** | Linux Kernel Community | `kernel.org` | GPL-2.0 Only | **STRICTLY PROHIBITED FROM IMPORT**. Consulted solely for high-level conceptual understanding of Buffer Objects and Fences | **PROHIBITED** (Conceptual Study Only) |

---

## 3. Detailed Forensic Findings by Vendor

### 3.1 Intel Media Stack
- The modern Intel user-space driver (`intel-media-driver` or `iHD`) is released under the permissive **MIT License**.
- It provides complete open definitions of MFX, HCP, and VDBox command opcodes (`MI_BATCH_BUFFER_START`, `MFX_PIPE_MODE_SELECT`, `MFX_AVC_IMG_STATE`, `MFX_AVC_SLICE_STATE`).
- Reusing these packet structures and definitions in ATOMS OS is 100% legally and architecturally compatible.
- **Firmware Consideration**: GuC/HuC microcodes from `linux-firmware` are proprietary binary blobs with redistribution licenses. In ATOMS OS, Gen7/Gen7.5 (Haswell) requires zero firmware, while modern generations can run in direct MMIO submission mode.

### 3.2 NVIDIA Media Stack
- NVIDIA's open kernel modules (`open-gpu-kernel-modules`) are explicitly dual-licensed under **MIT and GPL-2.0**. ATOMS OS exercises the MIT option.
- The user-space NVDEC client interface headers (`nvcuvid.h`, `cuviddec.h`) are published by NVIDIA with permissive rights allowing developers to build compatible callers without royalty.
- ATOMS OS develops its own native driver harness communicating with NVDEC channels, maintaining complete separation from proprietary user-space runtimes.

### 3.3 AMD Media Stack
- AMD's user-space hardware decoding library (`rocDecode`) is licensed under the permissive **MIT License**.
- The hardware interface for AMD Video Core Next (VCN) is openly documented in Mesa's `src/gallium/drivers/radeonsi/` (also MIT License).
- All command packet structures for VCN Indirect Buffers (IB) are permissive and safe for inclusion in ATOMS OS.

---

## 4. Architectural Rules for Engineers

1. **No Copy-Paste of Linux Kernel Code**: Never copy functions from `drivers/gpu/drm/` or `drivers/gpu/drm/i915/`. All BOS code must be written natively to conform to BOS conventions (`bos_video_*`, `bos_gpu_*`).
2. **Permissive Headers Only**: Only headers licensed under MIT, BSD-2/3-Clause, Apache-2.0, or CC0 may be used as references.
3. **Firmware Isolation**: Any binary microcode must be stored in isolated data partitions with explicit licensing documentation.

---

## 5. Certification Verdict

**AUDIT VERDICT: PASS**  
The proposed universal hardware video acceleration architecture respects all licensing constraints and introduces zero copyleft liabilities into ATOMS OS.
