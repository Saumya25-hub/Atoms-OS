# BOS GPU Driver V6 — NVIDIA Native Display Driver Architecture & Technical Specification

## 1. Executive Overview & Scope

This specification details the hardware architecture, register maps, display engine pipelines, and VRAM memory layout for the **BOS NVIDIA Native Display Subsystem (Phase 6)** based on official references from the **NVIDIA Open GPU Kernel Modules** and architectural design patterns from **Nouveau**.

### Strict Scope Directives

- **Target Scope**: Display Driver Only (PCI Detection, MMIO, Modesetting, VRAM Framebuffer, Hardware Cursor, Atomic Page Flipping, VBlank Sync, 2D Blitter, Diagnostics).
- **Explicit Non-Goals**: No CUDA, No Vulkan, No OpenGL ICD, No PhysX, No Tensor Cores, No Ray Tracing, No NVENC/NVDEC.

---

## 2. Hardware Architecture & Supported Families

NVIDIA GPU hardware is structured into microarchitectural families:

| Family / Architecture | Code Name | Generation | Key Features |
|---|---|---|---|
| **Tesla / Fermi** | G80..GT200 / GF100..GF119 | Gen8 / Gen9 | PRAMDAC / PCRTC Legacy Display Architecture |
| **Kepler / Maxwell** | GK104..GK208 / GM107..GM206 | Gen10 / Gen11 | EVO Display Engine (PDISP) |
| **Pascal** | GP102..GP108 (GTX 1050/1060/1070/1080) | Gen12 | Display Engine v1.0, 4K Scanout |
| **Turing** | TU102..TU117 (GTX 1650/1660, RTX 2060/2070/2080) | Gen13 | Display Core v2.0, Open Kernel Module Architecture |
| **Ampere / Ada / Hopper / Blackwell** | GA102..GA107 / AD102..AD107 (RTX 30/40/50 series) | Gen14 / Gen15 | GSP Engine Interface, Display Engine v3.0 |

---

## 3. PCI Device Identification Matrix

NVIDIA GPUs use Vendor ID **`0x10DE`**. Device IDs map to hardware generations:

```
NVIDIA PCI Vendor ID: 0x10DE

Pascal (GTX 1050 / 1060 / 1070 / 1080):
  0x1C81, 0x1C82, 0x1060, 0x1B81, 0x1B80

Turing (GTX 1650 / 1660, RTX 2060 / 2070 / 2080):
  0x1F82, 0x2184, 0x1F08, 0x1E84, 0x1E82

Ampere / Ada Lovelace (RTX 3050 / 3060 / 3070 / 3080 / 4060 / 4070 / 4080 / 4090):
  0x2507, 0x2487, 0x2206, 0x2204, 0x2786, 0x2704, 0x2684

Quadro / Professional Displays:
  0x1BB0, 0x1BB1, 0x1EB0, 0x2230
```

---

## 4. BAR Layout & MMIO Architecture

NVIDIA GPUs export two primary 64-bit PCI BARs:

```
+-----------------------------------------------------------------------+
|  PCI BAR 0: MMIO Control Region (Register Space)                     |
|  Size: 16 MB (0x01000000 bytes)                                       |
|                                                                       |
|  0x000000 - 0x000FFF : NV_PMC    (Power Management & Device Control)  |
|  0x001000 - 0x001FFF : NV_PBUS   (PCI Bus Interface & Config Controls)|
|  0x100000 - 0x100FFF : NV_PFB    (Framebuffer & Memory Controller)   |
|  0x600000 - 0x600FFF : NV_PCRTC  (CRTC Display Timing & Framebase)    |
|  0x610000 - 0x61FFFF : NV_PDISP  (Display Core Engine EVO / HUB)      |
|  0x680000 - 0x680FFF : NV_PRAMDAC(PRAMDAC & Hardware Cursor Engine)   |
+-----------------------------------------------------------------------+
|  PCI BAR 1: VRAM Framebuffer Aperture Window                         |
|  Size: 256 MB - 24 GB                                                 |
|                                                                       |
|  Direct CPU mapping to GPU VRAM Framebuffer & Surface Buffers.       |
+-----------------------------------------------------------------------+
```

---

## 5. Display Engine Architecture

The NVIDIA Display Engine manages display heads, timing control, primary planes, and hardware cursors:

```
+-----------------------------------------------------------------------+
|  VRAM FRAMEBUFFER (BAR 1 Aperture Mapped Surface)                    |
+-----------------------------------------------------------------------+
                                  |
                                  v
+-----------------------------------------------------------------------+
|  PRIMARY GRAPHICS PLANE (NV_PCRTC)                                    |
|  - NV_PCRTC_START                     (0x600800) : Base VRAM Offset   |
|  - NV_PCRTC_TIMING                    (0x600804) : 1920x1080 Timings  |
|  - NV_PCRTC_RASTER_START              (0x60080C) : VBlank Raster Line |
+-----------------------------------------------------------------------+
                                  |
                                  v
+-----------------------------------------------------------------------+
|  HARDWARE CURSOR PLANE (NV_PRAMDAC)                                   |
|  - NV_PRAMDAC_CURSOR_CTRL             (0x680800) : Enable & Format    |
|  - NV_PRAMDAC_CURSOR_BASE             (0x680804) : Cursor VRAM Base   |
|  - NV_PRAMDAC_CURSOR_POS              (0x680808) : X/Y Screen Coords  |
+-----------------------------------------------------------------------+
                                  |
                                  v
+-----------------------------------------------------------------------+
|  PRAMDAC / FLAT PANEL TIMING GENERATOR                                |
|  - NV_PRAMDAC_GENERAL_CONTROL         (0x680848) : Enable DAC & BPP   |
+-----------------------------------------------------------------------+
```

---

## 6. Modesetting Initialization Sequence

1. **Power Enable**: Write `0xFFFFFFFF` to `NV_PMC_ENABLE` (`0x000200`) to power display logic.
2. **Memory Controller Setup**: Query `NV_PFB_SIZE` (`0x100209`) for VRAM capacity.
3. **CRTC Timing Configuration**: Set 1920x1080 timing parameters in `NV_PCRTC_TIMING` (`0x600804`).
4. **Primary Framebase Setup**: Write VRAM physical offset to `NV_PCRTC_START` (`0x600800`).
5. **Display DAC Enable**: Write 32bpp format mode to `NV_PRAMDAC_GENERAL_CONTROL` (`0x680848`).

---

## 7. Atomic Page Flipping & VBlank Interrupts

1. Driver renders back buffer into VRAM.
2. Driver updates `NV_PCRTC_START` (`0x600800`) with new frame address.
3. Display engine latches new base address on next VBlank raster scanline.
4. Raster status line is monitored via `NV_PCRTC_RASTER_START` (`0x60080C`).

---

## 8. Sub-Phase Implementation Roadmap

- **Phase 6B**: NVIDIA PCI Detection & Auto-Binding (`0x10DE`)
- **Phase 6C**: MMIO BAR Discovery & Framebuffer Controller
- **Phase 6D**: Display Engine & Modesetting Setup (1920x1080)
- **Phase 6E**: Hardware Cursor Plane & VRAM Manager
- **Phase 6F**: Hardware Presentation & Atomic Page Flipping
- **Phase 6G**: Production Diagnostics Report
- **Phase 6H**: 30-Test Certification Suite (`Passed = 30, Failed = 0`)

---

## 9. Verification & Success Criteria

- NVIDIA GPU is detected automatically via PCI Vendor `0x10DE`.
- BAR0 MMIO (`16 MB`) and BAR1 VRAM Aperture map without faults.
- Display engine initializes to 1920x1080 resolution.
- `bos_gpu_present()` performs atomic page flipping over VRAM.
- All 30 unit tests pass (`Passed=30 Failed=0`).
