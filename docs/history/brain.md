# SignaturesOS Architectural Master Reference (`brain.md`)

> **IMPORTANT FOR AI AGENTS & DEVELOPERS**: Read this file first before making any modifications to SignaturesOS. This document maps out the exact subsystem boundaries, architectural rules, memory layout, and engine pipelines. Adhering to these specifications prevents architectural regression and ensures instant onboarding without wasting token budget.

---

## 1. Core Architectural Philosophy & Pipeline

SignaturesOS enforces rigid separation of concerns across graphics, input, storage, and windowing. **Never bypass intermediate subsystems.**

### Graphical & Asset Pipeline
```text
[Disk File / Fallback]
         ↓
  Virtual File System (VFS)
         ↓
  BOASSET ENGINE v1 (Resource Manager & Ref-Counted Cache)
         ↓
  BOIMAGE v2.5 QUALITY ENGINE (Sampling -> Scaling -> Alpha Blending)
         ↓
  Texture Atlas & Batch Renderer (Batching up to 2048 Sprites)
         ↓
  BOHEART (Rigid 60 FPS / 16.6ms Fixed Heartbeat Clock)
         ↓
  Hardware Framebuffer (VRAM via VBE)
```

---

## 2. Directory & Module Tree

```text
Signatures_OS/
├── arch/x86_64/              # Hardware interrupts (IDT, ISRs, IRQs, PIC, PIT, Port IO)
├── boot/                     # Stage 1 (boot.asm) & Stage 2 (stage2.asm) bootloaders
├── bovisual/                 # Low-level graphics foundation (PutPixel, Fill, Clear, Fonts)
├── drivers/
│   ├── input/                # USB Tablet absolute pointer & PS/2 mouse drivers
│   └── video/                # VBE Framebuffer & VGA drivers
├── kernel/
│   ├── boasset/              # BOASSET v1: Centralized resource manager & fallback synthesis
│   ├── boimage/              # BOIMAGE v2.5: Quality Engine, Atlas, Batch Queue, Decoder
│   ├── BOSurface/            # BOSurface & BWE: Windowing, Z-Order, Focus, Compositor
│   ├── input/                # BOMOUSETABUNDER & BMDE: Input abstraction & diagnostics
│   ├── memory/               # PMM (Bitmap), VMM (4-level paging), Heap v1 (4MB pool)
│   ├── scheduler/            # Preemptive Task Scheduler & Context Switcher
│   ├── vfs/                  # Virtual File System & FAT32 mount manager
│   └── kernel.c              # Kernel entry point & BOHEART main loop
├── userspace/                # ELF programs (Explorer, Terminal, Shell, libbos)
└── build.ps1                 # Automated PowerShell build & linkage pipeline
```

---

## 3. Subsystem Detailed Specifications

### A. BOASSET ENGINE v1 (`kernel/boasset/`)
- **Role**: Resource manager and central cache sitting directly above `BOIMAGE`.
- **Key Files**: `asset_types.h`, `boasset.h/.c`, `asset_cache.h/.c`, `asset_loader.h/.c`.
- **Core Rules**: UI code must **never** hardcode raw pixel buffers or load direct disk paths repeatedly. Request assets via `BOAsset_Get(ASSET_ID)` or draw via `BOAsset_DrawAsset(...)`.
- **Cache**: 256-entry lookup table. If an asset is already loaded, increments `ref_count` instead of reallocating heap memory.
- **Resilient Fallback**: If disk files are unpopulated during development, `asset_loader.c` automatically synthesizes crisp vector fallback icons in memory.

### B. BOIMAGE v2.5 QUALITY ENGINE (`kernel/boimage/`)
- **Role**: High-fidelity image decoding, texture caching, atlas packing, and sub-pixel resampling.
- **Modular Internal Layers**:
  1. **Sampling Engine (`BOImage_SamplePixel`)**: Fast 8-bit fixed-point Bilinear interpolation (`BO_FILTER_BILINEAR`) evaluating 4 neighboring texels without floating-point division inside loops.
  2. **Blend Engine (`BOImage_BlendPixel`)**: Precise channel-by-channel alpha blending ensuring transparent neon/glowing PNG edges don't develop dark gray borders.
  3. **Snapping Engine (`BOImage_SnapBounds`)**: Aligns floating coordinates to exact integer screen boundaries to eliminate half-pixel blur.
  4. **Scaling & Raster Engine (`BOImage_DrawEx`)**: Arbitrary dimension rendering with selectable filtering (`BO_FILTER_NEAREST` or `BO_FILTER_BILINEAR`).
- **Atlas & Batching**: Packs textures into dynamic grid cells (`BOAtlas`) and queues draw commands into `BOBatch` (up to 2048 sprites) to minimize draw calls.

### C. BOSurface & BWE (`kernel/BOSurface/`)
- **Role**: Windowing system managing desktop surfaces, active Z-order hierarchies, input hit-testing, and dirty rectangle damage tracking.
- **Focus Engine**: Guarantees rigid focus transition (`Active -> Focus`). When a surface is destroyed, focus falls back deterministically to the previous active window in Z-order.

### D. BOHEART (`kernel/kernel.c`)
- **Role**: The heartbeat clock enforcing a rigid **60 FPS (16.6ms)** frame execution loop.
- **Execution Lifecycle per Tick**:
  1. **Input Capture Phase**: Drain raw mouse/keyboard events without UI state mutation.
  2. **Timer Check**: If `< 16ms` elapsed, `scheduler_yield()` to give CPU cycles to user tasks.
  3. **Atomic Frame Phase**: Flush damage regions, run `BWE_Compose()`, execute `BOImage_FlushBatch()`, and swap double buffers (`BOVISUAL_Graphics_SwapBuffers`).

### E. Memory Management (`kernel/memory/`)
- **PMM**: Bitmap-based physical frame allocator managing RAM up to top address detected by bootloader.
- **VMM**: x86_64 4-level paging (`PML4 -> PDPT -> PD -> PT`). Maps kernel to virtual space and isolates back buffers (`0x90000000ULL`).
- **Heap v1**: Fixed pool allocator starting at `0x80000000` with a **4MB limit** (`0x80400000`).

---

## 4. Kernel Boot Flow & Initialization Order

When SignaturesOS boots via `build.ps1` into QEMU, initialization proceeds strictly in this order inside `kernel_main` ([kernel.c](file:///d:/Signatures_OS/kernel/kernel.c)):
1. **Hardware & Exceptions**: GDT, IDT, PIC (Cascade IRQ2 unmasked), ISRs.
2. **Input Stack**: `BOMOUSETABUNDER`, `USB-TABLET` (Absolute pointer driver), PS/2 controller.
3. **Memory Stack**: PMM bitmap calculation -> VMM paging setup -> Heap v1 initialization (`4MB pool`).
4. **Storage & VFS**: Disk manager -> ATA driver -> VFS registration -> Mount FAT32 root partition `/`.
5. **Asset Engine**: `BOAsset_Initialize()` -> `BOAsset_PreloadCritical()` (Preloads `ASSET_LOGO`, `ICON_FOLDER`, etc.).
6. **Multitasking & Graphics**: Preemptive scheduler init -> VBE hardware framebuffer mapping -> Back buffer allocation -> Register `kernel_main` as PID 2 boot task -> Enable interrupts (`sti`).
7. **Enter BOHEART Loop**: Rigid 60 FPS synchronization loop starts.

---

## 5. Build & Verification Guide

To compile and verify SignaturesOS:
```powershell
# Run the automated build script from workspace root
powershell -ExecutionPolicy Bypass -File .\build.ps1
```
- **Validation**: Script checks boot sector signatures, LBA kernel offsets, sector alignment (64MB image), and outputs `build\SignaturesOS.vdi`.
- **Run in QEMU**:
```cmd
qemu-system-x86_64.exe -m 1024 -machine q35 -cpu max -smp 2 -vga std -display gtk -usb -device usb-tablet -drive file="D:\Signatures_OS\build\OS.img",format=raw,if=ide -serial stdio
```
