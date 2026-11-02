# ATOMS OS — FORENSIC INVESTIGATION REPORT (TASK 1)
**Subsystems:** 
- Phase 5: IEC (Input Event Core & Nano-Ring) & 12-Stage PPE V2 (Precision Pointer Engine V2)
- Phase 6: CPE (Universal Hardware Cursor Plane Handler: Intel, AMD, NVIDIA, VirtIO, VMware)
**Authority:** ATOMS OS Core Architecture Group  
**Compliance:** Rule 0 Phase Isolation (NO CODE MODIFICATIONS IN THIS STEP)

---

## 1. INVESTIGATION OBJECTIVE

Investigate the existing ATOMS OS pointer pipeline, event queueing, and GPU cursor planes across both physical bare-metal hardware (ASUS PRIME B760M-K / Raptor Lake Refresh, Haswell H81) and hypervisor targets (QEMU, VMware).
Identify bottlenecks, missing interfaces, and architectural gaps in:
1. High-frequency pointer queueing and event delivery.
2. 12-stage sub-pixel accumulation, ballistic acceleration curves, and kinetic wheel filtering.
3. GPU hardware cursor plane MMIO integration across Intel, AMD, NVIDIA, and VirtIO.

---

## 2. FORENSIC EVIDENCE & CODEBASE AUDIT

### Evidence Item A: Pointer Presentation Bottleneck in Software Fallback
- **Location:** `kernel/graphics/BSPE/Cursor/bspe_cursor_present.c:218` (`BSPE_CursorPresenter_FastTileUpdate()`)
- **Observed Behavior:**
  When a mouse event arrives, `BSPE_CursorPresenter_FastTileUpdate()` performs a software read-modify-write on VRAM over PCI:
  1. Restores the 64x64 background tile from RAM.
  2. Blends the 32-bit ARGB cursor sprite with alpha calculation.
  3. Writes 4KB of pixel data across the PCIe bus to physical VRAM.
- **Root Problem:**
  On physical hardware with high-resolution displays (2560x1600 or 4K), software tile blits consume CPU cycles and PCIe bus bandwidth on every single mouse packet. If the desktop compositor or a heavy userspace task runs, cursor motion can hitch or drop frames.

### Evidence Item B: GPU Driver Cursor Operations Are NULL
- **Locations:**
  - `kernel/graphics/gpu/drivers/gpu_drv_intel.c:322` (`g_intel_driver.ops`)
  - `kernel/graphics/gpu/drivers/gpu_drv_amd.c:377` (`g_amd_driver.ops`)
  - `kernel/graphics/gpu/drivers/gpu_drv_nvidia.c:290` (`g_nvidia_driver.ops`)
- **Observed Behavior:**
  While `bos_gpu_driver_ops_t` (in `gpu_driver.h:35-37`) defines `set_cursor_position`, `set_cursor_image`, and `set_cursor_visibility`, and while diagnostic strings report `Hardware Cursor Plane: READY`, the actual function pointers in Intel, AMD, and NVIDIA drivers are currently **NULL** (only `gpu_drv_vmware.c` implemented them).
- **Impact:**
  Because the ops are NULL, `cursor_backend_is_hardware()` always returns `false` on physical Intel (i3-14100F UHD 730, Haswell HD 4400), AMD, and NVIDIA GPUs, forcing 100% reliance on software VRAM blits.

### Evidence Item C: Register Definitions Already Declared
- **Intel:** `gpu_intel_regs.h:40-42` defines `INTEL_CURACNTR` (0x70080), `INTEL_CURABASE` (0x70084), `INTEL_CURAPOS` (0x70088).
- **AMD:** `gpu_amd_regs.h:16-18` defines `AMD_D1CUR_CONTROL` (0x0A00), `AMD_D1CUR_SURFACE_ADDRESS` (0x0A04), `AMD_D1CUR_POSITION` (0x0A08).
- **NVIDIA:** `gpu_nvidia_regs.h:26-28` defines `NV_PRAMDAC_CURSOR_CTRL` (0x680800), `NV_PRAMDAC_CURSOR_BASE` (0x680804), `NV_PRAMDAC_CURSOR_POS` (0x680808).
- **Status:** The hardware register map is already verified and available. The missing link is the driver implementation and memory mapping of the cursor sprite buffer.

### Evidence Item D: Event Ingestion & Decoupling Gap
- **Location:** `kernel/drivers/input/core/hida.c:274` (`hida_push_relative`)
- **Observed Behavior:**
  When a mouse report arrives from xHCI MSI-X interrupt, `hida_push_relative` synchronously pushes directly into `input_core` and invokes `input_core_dispatch_events()`, which directly executes `pointer_motion_process()`.
- **Root Problem:**
  Processing motion math, bounds clamping, consumer notifications, and presenter calls synchronously inside or directly chained to the interrupt handler risks latency spikes under bursty 1000Hz gaming mice traffic. A lockless static Nano-Ring (`AIIE_NanoRing`) with atomic `EV_SYN` batching is required to decouple ingest from evaluation.

---

## 3. SUSPECTED ARCHITECTURAL FIXES

1. **Phase 5: IEC & 12-Stage PPE V2**
   - Create a lockless static Nano-Ring (`AIIE_NanoRing`, 512 entries, 0 heap) in `kernel/drivers/input/core/`.
   - Bundle simultaneous relative movement ($dX, dY$) and button updates atomically via `AIIE_EV_SYN`.
   - Upgrade the 7-stage motion pipeline in `kernel/drivers/input/pointer/pointer_motion.c` to the full 12-stage precision engine (including spike filtration, velocity differentiation, 16.16 sub-pixel accumulation, kinetic wheel deceleration, and multi-monitor edge clamping).

2. **Phase 6: CPE Universal Hardware Cursor Plane Handler**
   - **Intel Driver (`gpu_drv_intel.c`):**
     Implement `intel_set_cursor_position`, `intel_set_cursor_image`, `intel_set_cursor_visibility`.
     Allocate dedicated 16KB 64x64 ARGB cursor sprite in GGTT / Stolen VRAM.
     Program `INTEL_CURAPOS`, `INTEL_CURABASE`, `INTEL_CURACNTR`.
   - **AMD Driver (`gpu_drv_amd.c`):**
     Implement `amd_set_cursor_position`, `amd_set_cursor_image`, `amd_set_cursor_visibility`.
     Program `AMD_D1CUR_POSITION`, `AMD_D1CUR_SURFACE_ADDRESS`, `AMD_D1CUR_CONTROL`.
   - **NVIDIA Driver (`gpu_drv_nvidia.c`):**
     Implement `nv_set_cursor_position`, `nv_set_cursor_image`, `nv_set_cursor_visibility`.
     Program `NV_PRAMDAC_CURSOR_POS`, `NV_PRAMDAC_CURSOR_BASE`, `NV_PRAMDAC_CURSOR_CTRL`.
   - **Hardware Detection Bridge (`cursor_backend.c` & `cursor_hal.c`):**
     Connect `cursor_backend_is_hardware()` to probe `bos_gpu_get_primary()`. If the primary GPU supports hardware cursor ops, enable hardware mode with 0% CPU. If unsupported or on GOP fallback, retain seamless BSPE software fast micro-tile.

---

## 4. RISK ANALYSIS

- **GPU MMIO Isolation:** On machines running under pure UEFI GOP without initialized GPU display engines, attempting unmapped MMIO writes could fault. The driver MUST check `dev->mmio_virt != NULL` and verify device presence before register access.
- **Fail-Safe Fallback:** If hardware cursor plane initialization fails or coordinates are rejected, the engine MUST fall back to `BSPE_CursorPresenter_FastTileUpdate()` without losing cursor visibility.
- **Zero Heap Requirement:** All Nano-Ring structures and cursor sprite buffers MUST be pre-allocated in static BSS or page-aligned physical buffers. No `kmalloc` in the interrupt or fast path.
