# BOS GPU Driver Subsystem — Phase 1 Architecture & Specifications

## Overview

Phase 1 of the BOS GPU Driver Subsystem establishes the foundational **Hardware Abstraction Layer (HAL)**, PCI graphics device discovery, driver registration pipeline, capability query matrix, surface lifecycle abstractions, software rendering fallback backend, and diagnostic logging.

This subsystem resides cleanly in:
`kernel/graphics/gpu/`

---

## Directory & Subsystem Structure

```
kernel/graphics/gpu/
├── include/
│   ├── gpu.h             # Central HAL public API & subsystem types
│   ├── gpu_caps.h        # 64-bit capability bitfield flags
│   ├── gpu_device.h      # BOSGPUDevice object & BAR definitions
│   ├── gpu_driver.h      # Driver interface table & operational hooks
│   └── gpu_surface.h     # HAL Surface descriptor & pixel formats
├── core/
│   └── gpu_manager.c     # GPU Manager, driver binding & software fallback
├── pci/
│   └── gpu_pci.c         # PCI bus scanner for Class 0x03 Display Controllers
├── memory/
│   └── gpu_memory.c      # VRAM allocation & MMIO BAR mapping abstractions
├── surface/
│   └── gpu_surface.c     # Surface lifecycle (creation, destruction, mapping)
├── drivers/
│   ├── gpu_drv_vmware.c  # VMware SVGA II Placeholder Driver (Vendor 0x15AD)
│   ├── gpu_drv_virtio.c  # VirtIO GPU Placeholder Driver (Vendor 0x1AF4)
│   ├── gpu_drv_intel.c   # Intel Graphics Placeholder Driver (Vendor 0x8086)
│   ├── gpu_drv_amd.c     # AMD Radeon Placeholder Driver (Vendor 0x1002)
│   ├── gpu_drv_nvidia.c  # NVIDIA GeForce Placeholder Driver (Vendor 0x10DE)
│   └── gpu_drv_swrender.c# Software Renderer Fallback Driver (Vendor 0xFFFF)
├── debug/
│   └── gpu_debug.c       # Powerful diagnostic logging & capability dumps
└── tests/
    └── gpu_tests.c       # Unit test suite verifying subsystem HAL
```

---

## HAL Interface Contract

Every GPU driver in BOS OS must implement the standard `bos_gpu_driver_ops_t` function table:

```c
typedef struct bos_gpu_driver_ops {
    bos_gpu_status_t (*init)(bos_gpu_device_t* dev);
    bos_gpu_status_t (*shutdown)(bos_gpu_device_t* dev);
    bos_gpu_status_t (*present)(bos_gpu_device_t* dev, bos_gpu_surface_t* surface);
    bos_gpu_status_t (*create_surface)(bos_gpu_device_t* dev, uint32_t width, uint32_t height, uint32_t format, bos_gpu_surface_t** out_surf);
    bos_gpu_status_t (*destroy_surface)(bos_gpu_device_t* dev, bos_gpu_surface_t* surface);
    bos_gpu_status_t (*map)(bos_gpu_device_t* dev, bos_gpu_surface_t* surface, void** out_ptr);
    bos_gpu_status_t (*unmap)(bos_gpu_device_t* dev, bos_gpu_surface_t* surface);
    bos_gpu_status_t (*fill_rect)(bos_gpu_device_t* dev, bos_gpu_surface_t* surf, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
    bos_gpu_status_t (*copy)(bos_gpu_device_t* dev, bos_gpu_surface_t* src, bos_gpu_surface_t* dst, uint32_t sx, uint32_t sy, uint32_t dx, uint32_t dy, uint32_t w, uint32_t h);
    bos_gpu_status_t (*stretch_copy)(bos_gpu_device_t* dev, bos_gpu_surface_t* src, bos_gpu_surface_t* dst, uint32_t sx, uint32_t sy, uint32_t sw, uint32_t sh, uint32_t dx, uint32_t dy, uint32_t dw, uint32_t dh);
    bos_gpu_status_t (*wait_idle)(bos_gpu_device_t* dev);
    bos_gpu_status_t (*get_caps)(bos_gpu_device_t* dev, uint64_t* caps);
} bos_gpu_driver_ops_t;
```

---

## Core Public HAL APIs (`gpu.h`)

- `bos_gpu_init()`: Bootstraps the GPU Manager, registers standard drivers, scans PCI for display adapters, binds drivers, and establishes primary GPU or software fallback.
- `bos_gpu_shutdown()`: Gracefully releases all device contexts and drivers.
- `bos_gpu_enumerate()`: Returns the number of registered GPU devices.
- `bos_gpu_register_driver(driver)`: Registers a new GPU driver with the Manager.
- `bos_gpu_get_primary()`: Returns pointer to active primary `bos_gpu_device_t`.
- `bos_gpu_get_caps(dev, out_caps)`: Retrieves capability bitmask for specified or primary GPU.
- `bos_gpu_present(surface)`: Flushes/presents surface scanout buffer to display.
- `bos_gpu_run_tests()`: Executes Phase 1 unit test suite.

---

## Capability System Flags (`gpu_caps.h`)

| Capability Flag | Hex Value | Description |
|---|---|---|
| `BOS_GPU_CAP_FRAMEBUFFER` | `0x0001` | Linear frame buffer access |
| `BOS_GPU_CAP_VRAM` | `0x0002` | Dedicated Video RAM |
| `BOS_GPU_CAP_DMA` | `0x0004` | Direct Memory Access engine |
| `BOS_GPU_CAP_BLITTER` | `0x0008` | 2D Hardware BitBLT engine |
| `BOS_GPU_CAP_SCALING` | `0x0010` | Hardware image scaling |
| `BOS_GPU_CAP_OVERLAY` | `0x0020` | Video hardware overlay plane |
| `BOS_GPU_CAP_HW_CURSOR` | `0x0040` | Dedicated Hardware Cursor Plane |
| `BOS_GPU_CAP_PAGE_FLIP` | `0x0080` | Hardware Page Flipping |
| `BOS_GPU_CAP_DOUBLE_BUFFER` | `0x0100` | Double Buffering support |
| `BOS_GPU_CAP_TRIPLE_BUFFER` | `0x0200` | Triple Buffering support |
| `BOS_GPU_CAP_VSYNC` | `0x0400` | CRTC Vertical Synchronization |

---

## Boot Sequence Integration

During OS Boot in `kernel/kernel.c`:
1. `pci_init()` initializes PCI bus topology.
2. `bos_gpu_init()` initializes the GPU subsystem:
   - Prints `[GPU LOG] GPU Manager Started`
   - Scans PCI bus for Class `0x03` display controllers.
   - Binds drivers (or loads Software Renderer fallback if no hardware driver claims device).
   - Sets primary active GPU device.
3. `bos_gpu_print_diagnostics()` dumps device topology & capability matrix.
4. `bos_gpu_run_tests()` verifies HAL surface allocation, driver registration, capability queries, and fallback logic.
