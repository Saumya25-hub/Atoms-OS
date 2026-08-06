# BOS GPU Driver V5 — AMD Radeon Native Graphics Driver Architecture & Technical Specification

## 1. Executive Overview

This specification details the hardware architecture, register layouts, GART translation tables, Display Core (DC) pipelines, and SDMA command submission engines for the **BOS AMD Radeon Graphics Subsystem (Phase 5)**.

The driver provides native hardware acceleration for AMD Radeon graphics processors across **GCN (Graphics Core Next)** and **RDNA (Radeon DNA)** microarchitectures.

---

## 2. Hardware Architecture & Supported GPU Families

AMD GPUs are grouped into major microarchitectural families:

| GPU Family | Architecture Generation | Typical Models | Display Engine |
|---|---|---|---|
| **GCN 1.0 / 2.0** | Southern / Sea Islands | HD 7700/7800/7900, R7 260, R9 290 | DCE 6.0 / DCE 8.0 |
| **GCN 3.0 / 4.0** | Volcanic Islands / Polaris | R9 285/380, RX 470/480/570/580/590 | DCE 10.0 / DCE 11.2 |
| **GCN 5.0** | Vega | RX Vega 56/64, Radeon VII, Vega 10/20 | DCN 1.0 / DCN 2.0 |
| **RDNA 1.0 / 2.0** | Navi 1x / Navi 2x | RX 5500/5600/5700, RX 6600/6700/6800/6900 | DCN 2.0 / DCN 3.0 |
| **RDNA 3.0** | Navi 3x | RX 7600/7700/7800/7900 XT/XTX | DCN 3.2 |

---

## 3. PCI Device Identification Matrix

AMD GPUs use Vendor ID **`0x1002`**. Device IDs map to GPU family generations:

```
AMD PCI Vendor ID: 0x1002

GCN 1.0 / 2.0 (Southern / Sea Islands):
  0x6798, 0x679A, 0x67B0, 0x67B1, 0x6610, 0x6611

GCN 4.0 (Polaris 10/20/30):
  0x67DF, 0x67C0, 0x67EF, 0x67FF, 0x6FDF (RX 470/480/570/580/590)

GCN 5.0 (Vega 10/20):
  0x687F, 0x6860, 0x6861, 0x66AF (Vega 56/64, Radeon VII)

RDNA 1.0 (Navi 10/14):
  0x731F, 0x7340, 0x7360 (RX 5700 / 5500 XT)

RDNA 2.0 (Navi 21/22/23):
  0x73A0, 0x73BF, 0x73DF, 0x743F (RX 6900 XT / 6800 XT / 6700 XT)

RDNA 3.0 (Navi 31/32/33):
  0x744C, 0x7479, 0x7480 (RX 7900 XTX / 7800 XT)
```

---

## 4. BAR Layout & Memory Architecture

AMD Radeon GPUs expose two primary 64-bit PCI BARs:

```
+-----------------------------------------------------------------------+
|  PCI BAR 0: MMIO Registers & Control Block                           |
|  Size: 256 KB - 8 MB (0x00040000 - 0x00800000 bytes)                  |
|                                                                       |
|  0x0000 - 0x1FFF : Display Engine (DCE / DCN) Control Registers       |
|  0x2000 - 0x3FFF : Memory Controller (MC / GMC) & GART Registers      |
|  0x4000 - 0x5FFF : SDMA Engine Registers                             |
+-----------------------------------------------------------------------+
|  PCI BAR 2: VRAM Aperture Window                                     |
|  Size: 256 MB - 16 GB                                                 |
|                                                                       |
|  Direct CPU mapping to GPU VRAM Framebuffer & Surface Buffers.       |
+-----------------------------------------------------------------------+
```

### Memory Subsystems

1. **VRAM (Video RAM)**: High-performance GDDR5/GDDR6/HBM dedicated graphics memory.
2. **Visible VRAM**: The portion of VRAM directly mapped via BAR 2.
3. **GART (Graphics Address Translation Table)**: 4 KB page table in VRAM mapping system RAM pages into GPU physical address space.

---

## 5. Display Engine & Display Core (DC) Architecture

The AMD Display Engine contains Display Pipes, Primary Graphics Planes, and Hardware Cursor Planes:

```
+-----------------------------------------------------------------------+
|  VRAM FRAMEBUFFER (BAR 2 Aperture Mapped Surface)                    |
+-----------------------------------------------------------------------+
                                  |
                                  v
+-----------------------------------------------------------------------+
|  PRIMARY GRAPHICS PLANE (D1GRPH / HUBP)                               |
|  - D1GRPH_CONTROL                      (0x0500) : Format & Enable    |
|  - D1GRPH_PRIMARY_SURFACE_ADDRESS      (0x050C) : Framebase VRAM Addr|
|  - D1GRPH_PITCH                        (0x0520) : Surface Stride     |
+-----------------------------------------------------------------------+
                                  |
                                  v
+-----------------------------------------------------------------------+
|  HARDWARE CURSOR PLANE (D1CUR)                                        |
|  - D1CUR_CONTROL                       (0x0A00) : Enable & Format    |
|  - D1CUR_SURFACE_ADDRESS               (0x0A04) : Cursor Buffer Addr |
|  - D1CUR_POSITION                      (0x0A08) : X/Y Screen Coords  |
+-----------------------------------------------------------------------+
                                  |
                                  v
+-----------------------------------------------------------------------+
|  DISPLAY PIPES & TIMING GENERATORS (CRTC1 / OPTC1)                    |
|  - CRTC_CONTROL                        (0x0600) : CRTC Timing Enable |
|  - CRTC_H_TOTAL                        (0x0604) : Active & Total H   |
|  - CRTC_V_TOTAL                        (0x0608) : Active & Total V   |
+-----------------------------------------------------------------------+
```

---

## 6. SDMA Engine & 2D Hardware Acceleration

Asynchronous memory transfer and blits use the **SDMA (System Direct Memory Access)** engine:

### SDMA Ring Registers

- `SDMA0_GFX_RB_BASE`: Physical base address of SDMA ring buffer.
- `SDMA0_GFX_RB_CNTL`: Enable bit & Ring buffer size.
- `SDMA0_GFX_RB_RPTR`: Read pointer updated by GPU.
- `SDMA0_GFX_RB_WPTR`: Write pointer updated by driver.

### SDMA Packet Opcodes

- `SDMA_OP_COPY` (`0x01`): Hardware memory block copy.
- `SDMA_OP_FILL` (`0x02`): Hardware 2D rectangle pattern fill.

---

## 7. Modesetting Initialization Pipeline

1. **GMC / Memory Setup**: Initialize VRAM aperture & GART page table boundaries.
2. **Display Clock Setup**: Enable Display PLL & DPREFCLK.
3. **CRTC Timing Setup**: Program `CRTC_H_TOTAL` (1920) & `CRTC_V_TOTAL` (1080).
4. **Primary Plane Control**: Configure `D1GRPH_CONTROL` (32bpp ARGB), write `D1GRPH_PRIMARY_SURFACE_ADDRESS`.
5. **Enable Display Engine**: Set enable bits in `CRTC_CONTROL`.

---

## 8. Atomic Page Flipping & VBlank Interrupts

1. Driver renders back buffer in VRAM.
2. Driver updates `D1GRPH_PRIMARY_SURFACE_ADDRESS` with new VRAM physical offset.
3. Display engine latches new surface address on the next VBlank interval.
4. Interrupt handler catches VBlank event signal to acknowledge flip completion.

---

## 9. Sub-Phase Implementation Roadmap

- **Phase 5B**: AMD PCI Detection & Auto-Binding (`0x1002`)
- **Phase 5C**: MMIO BAR Discovery & Memory Controller Setup
- **Phase 5D**: Display Core Engine & Modesetting Setup
- **Phase 5E**: VRAM & GART Memory Manager
- **Phase 5F**: Hardware Presentation & Atomic Page Flipping
- **Phase 5G**: Hardware Blitter & SDMA Engine (`FillRect`, `Copy`, `StretchCopy`)
- **Phase 5H**: Production Diagnostics Report
- **Phase 5I**: 24-Test Certification Suite (`Passed = 24, Failed = 0`)

---

## 10. Verification & Success Criteria

- AMD GPU is detected automatically via PCI Vendor `0x1002`.
- MMIO BAR0 and VRAM Aperture BAR2 map without faults.
- Display engine initializes to 1920x1080 resolution.
- SDMA engine executes hardware fills and surface copies.
- `bos_gpu_present()` performs atomic page flipping over VRAM.
- All 24 unit tests pass (`Passed=24 Failed=0`).
