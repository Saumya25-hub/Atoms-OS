# BOS GPU Driver V4 — Intel Native Graphics Driver Architecture & Technical Specification

## 1. Executive Overview

This document specifies the hardware architecture, register maps, memory translation structures, display engine pipelines, and command submission interfaces for the **BOS Intel Native Graphics Subsystem (Phase 4)**.

The driver provides native hardware acceleration for Intel integrated and discrete graphics processing units across **Intel HD Graphics**, **Intel UHD Graphics**, **Intel Iris**, and **Intel Iris Xe** microarchitectures.

---

## 2. Hardware Architecture & Supported Generations

Intel GPU hardware is organized into microarchitectural generations (Gen):

| Generation | Code Names / Processor Families | Key Features & Display Engine |
|---|---|---|
| **Gen6** | Sandy Bridge (HD 2000 / 3000) | Split display architecture, PCH Transcoder, PPGTT support |
| **Gen7 / 7.5** | Ivy Bridge, Haswell (HD 2500/4000/4600) | Power Wells, LCPLL, DDI Encoders, 3D Pipeline v7 |
| **Gen8** | Broadwell (HD 5300/5500/6000) | 64-bit Canonical Virtual Addressing, 48-bit PPGTT |
| **Gen9** | Skylake, Kaby Lake, Coffee Lake, Comet Lake (HD 520/620, UHD 620/630) | Universal Plane Architecture (`PLANE_CTL`), DBUF Display Buffer Allocation, CDCLK adjustment |
| **Gen11** | Ice Lake (Iris Plus 940) | Gen11 Display Engine, Dual Pipe Transcoders, Type-C DDI |
| **Gen12 (Xe-LP)** | Tiger Lake, Alder Lake, Rocket Lake, Iris Xe, DG1 (UHD 730/770, Xe Graphics) | Xe-LP 12th Gen Microarchitecture, Async Compute Engines, Tile-4 Format, DBUF Slice Split |

---

## 3. PCI Device Identification Matrix

Intel GPUs use Vendor ID **`0x8086`**. Device IDs map to microarchitecture generations:

```
Intel PCI Vendor ID: 0x8086

Gen6 (Sandy Bridge):
  0x0102, 0x0112, 0x0122, 0x0106, 0x0116, 0x0126

Gen7 / Gen7.5 (Ivy Bridge / Haswell):
  0x0152, 0x0162, 0x0156, 0x0166, 0x0412, 0x0416, 0x041E, 0x0A16, 0x0D26

Gen8 (Broadwell):
  0x1612, 0x1616, 0x161E, 0x1622, 0x1626

Gen9 / Gen9.5 (Skylake / Kaby Lake / Coffee Lake / Comet Lake):
  0x1912, 0x1916, 0x191B, 0x191E (Skylake HD 520 / 530)
  0x5912, 0x5916, 0x591B, 0x591E (Kaby Lake HD 620 / 630)
  0x3E91, 0x3E92, 0x3E9B, 0x3E94 (Coffee Lake UHD 630)
  0x9B41, 0x9BCA, 0x9BC5, 0x9BC8 (Comet Lake UHD 630)

Gen11 (Ice Lake):
  0x8A51, 0x8A52, 0x8A5A, 0x8A5C

Gen12 (Tiger Lake / Alder Lake Iris Xe / UHD 770):
  0x9A40, 0x9A49, 0x9A59, 0x9A60, 0x9A70, 0x4680, 0x4682, 0x4690, 0x46A0, 0x4905
```

---

## 4. BAR Layout & Memory Mapping

Intel GPUs export two primary 64-bit PCI BARs:

```
+-----------------------------------------------------------------------+
|  PCI BAR 0: GTTMMADR (Graphics Translation Table & MMIO Address Space)|
|  Size: 16 MB (0x01000000 bytes)                                       |
|                                                                       |
|  0x000000 - 0x7FFFFF (8 MB)  : MMIO Register Controls                 |
|  0x800000 - 0xFFFFFF (8 MB)  : GGTT Page Table Entries (PTE Array)    |
+-----------------------------------------------------------------------+
|  PCI BAR 2: GMADR (Graphics Memory Aperture / Linear VRAM Access)    |
|  Size: 256 MB - 512 MB (0x10000000 - 0x20000000 bytes)                |
|                                                                       |
|  Provides direct CPU access to framebuffers via GGTT mapping.        |
+-----------------------------------------------------------------------+
```

### Stolen Memory Registers

System BIOS reserves "Stolen Memory" at boot for GPU scanout framebuffers:
- **`BSM` (Base of Stolen Memory)**: PCI Configuration Register `0x5C` (or `0x70` in graphics host bridge). Reading `BSM & ~0xFFF` yields physical start address of GPU VRAM framebuffer.

---

## 5. Display Engine Architecture

The Intel Display Engine consists of **Pipes**, **Transcoders**, **Planes**, and **Encoders**:

```
+-----------------------------------------------------------------------+
|  SURFACE MEMORY (Stolen VRAM / GGTT Mapped Pages)                     |
+-----------------------------------------------------------------------+
                                  |
                                  v
+-----------------------------------------------------------------------+
|  PLANES (Plane 1 / Primary, Plane 2 / Sprite, Cursor Plane)           |
|  - PLANE_CTL_1_A   (0x70180) : Pixel Format, Enable, Color Space     |
|  - PLANE_SURF_1_A  (0x7019C) : Base Physical Surface Address           |
|  - PLANE_STRIDE_1_A(0x70188) : Surface Stride / Pitch                 |
|  - PLANE_SIZE_1_A  (0x70190) : Width x Height                         |
+-----------------------------------------------------------------------+
                                  |
                                  v
+-----------------------------------------------------------------------+
|  DISPLAY PIPES & TRANSCODERS (Pipe A, B, C, Transcoder A, B, EDP)     |
|  - PIPECONF_A      (0x70008) : Pipe Timing, Interlacing, Active State |
|  - HTOTAL_A        (0x60000) : Horizontal Total & Active              |
|  - VTOTAL_A        (0x60004) : Vertical Total & Active                |
+-----------------------------------------------------------------------+
                                  |
                                  v
+-----------------------------------------------------------------------+
|  ENCODERS & CONNECTORS (DDI A, DDI B, HDMI, DisplayPort, eDP)         |
|  - DDI_BUF_CTL_A   (0x64000) : DDI Buffer Enable & Lane Count        |
+-----------------------------------------------------------------------+
```

---

## 6. Global Graphics Translation Table (GGTT)

The GGTT translates 32-bit/64-bit GPU Virtual Addresses to Host Physical RAM addresses.

Each GGTT Page Table Entry (PTE) is 64-bit (8 bytes):

```
+-----------------------------------------------------------------------+
| Bit 63..12 : Physical Page Address (PhysAddr >> 12)                  |
| Bit 1      : Cacheability / Write-Combining Bit                       |
| Bit 0      : Valid / Present Flag (1 = Valid, 0 = Fault)              |
+-----------------------------------------------------------------------+
```

GGTT Array Base: `GTTMMADR + 0x800000` (8 MB offset into BAR0).

---

## 7. Modesetting Initialization Sequence

To initialize display output on Intel GPUs without BIOS dependency:

1. **Power Well Enable**: Write `0x80000000` to `PWR_WELL_CTL` (`0x45400`) to power display engine.
2. **CDCLK Setup**: Configure Core Display Clock in `CDCLK_CTL` (`0x46000`).
3. **DDI / DPLL Enable**: Enable Display Phase-Locked Loops (`DPLL_CTRL1` `0x6C058`).
4. **Pipe Configuration**: Set `HTOTAL_A`, `VTOTAL_A`, `HBLANK_A`, `VBLANK_A` timings.
5. **Transcoder Enable**: Assert `TRANS_ENABLE` in `TRANS_CONF_A` (`0x70008`).
6. **Plane Enable**: Configure `PLANE_CTL_1_A` (`0x70180`), `PLANE_STRIDE_1_A` (`0x70188`), write `PLANE_SURF_1_A` (`0x7019C`).

---

## 8. Command Submission (Ring Buffer Engine)

GPU command execution uses hardware Ring Buffers:
- **Render Ring (RCS)**: MMIO Base `0x02000`
- **Blitter Ring (BCS)**: MMIO Base `0x22000`
- **Video Ring (VCS)**: MMIO Base `0x12000`

### Ring Register Map

```
RING_TAIL  (Base + 0x30) : Driver writes next command byte offset
RING_HEAD  (Base + 0x34) : Hardware updates current execution offset
RING_START (Base + 0x38) : Physical base address of Ring Buffer
RING_CTL   (Base + 0x3C) : Enable bit & Ring Size in pages
```

---

## 9. Hardware Cursor Engine

Intel hardware cursor operates on a dedicated plane:
- **`CURACNTR` (`0x70080`)**: Cursor Control (Enable, Format 64x64 ARGB8888).
- **`CURABASE` (`0x70084`)**: Surface Base Address in GGTT.
- **`CURAPOS`  (`0x70088`)**: Position `(Y << 16) | X`.

---

## 10. Atomic Page Flipping & VBlank Interrupts

Atomic page flipping is performed without screen tearing:
1. Render new back-buffer frame into VRAM.
2. Write physical address of new frame to `PLANE_SURF_1_A` (`0x7019C`).
3. Hardware latches `PLANE_SURF_1_A` atomically on the next Vertical Blanking interval (VBlank).
4. VBlank Interrupts are signaled via `DEIIR` (`0x44008`) Bit 0 (`PIPE_A_VBLANK`).

---

## 11. Sub-Phase Implementation Roadmap

The implementation is structured into 8 modular sub-phases:

- **Phase 4A**: Intel PCI Detection & BAR/MMIO Discovery (`gpu_drv_intel.c`)
- **Phase 4B**: Display Engine Power & Clock Setup
- **Phase 4C**: Modesetting & Pipe/Transcoder Setup
- **Phase 4D**: Framebuffer & GGTT Memory Manager
- **Phase 4E**: Hardware Cursor Plane Driver
- **Phase 4F**: Page Flipping & VBlank Subsystem
- **Phase 4G**: Production Diagnostics Suite
- **Phase 4H**: 18-Test Intel Driver Certification Suite

---

## 12. Verification & Success Criteria

- Intel GPU is detected automatically via PCI Vendor `0x8086`.
- BAR0 MMIO and BAR2 Aperture map without faults.
- GGTT entries program successfully.
- Hardware display plane outputs 1920x1080 resolution.
- `bos_gpu_present()` performs atomic page flipping.
- All 18 unit tests pass (`Passed=18 Failed=0`).
