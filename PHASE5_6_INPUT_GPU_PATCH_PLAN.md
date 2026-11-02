# ATOMS OS — ARCHITECTURE & PATCH PLAN (TASK 2)
**Subsystems:**
- Phase 5: IEC (Nano-Ring) & 12-Stage PPE V2 (Precision Pointer Engine V2)
- Phase 6: CPE (Universal Hardware Cursor Plane Handler: Intel, AMD, NVIDIA, VirtIO, VMware)
**Input:** `PHASE5_6_INPUT_GPU_FORENSIC_REPORT.md`  
**Compliance:** Rule 0 Phase Isolation (NO CODE MODIFICATIONS IN THIS STEP)

---

## 1. OBJECTIVE

1. **IEC & Nano-Ring (Phase 5):**
   - Provide a lockless static circular Nano-Ring buffer (512 entries, zero dynamic heap allocation) that buffers high-frequency mouse reports (up to 1000Hz from xHCI MSI-X).
   - Implement `AIIE_EV_SYN` atomic event batching to ensure display compositors never evaluate half-frames.
2. **12-Stage PPE V2 (Phase 5):**
   - Full 12-stage deterministic motion pipeline:
     1. Ingest from Nano-Ring.
     2. Spike / glitch filter (reject $> \pm 8192$ units).
     3. High-precision velocity estimation.
     4. Configurable polynomial acceleration curves (Raw, Linear, Enhanced Precision).
     5. 16.16 fixed-point sub-pixel accumulator.
     6. Canonical to display geometry scaling.
     7. Multi-monitor edge clamping.
     8. Debounced button state machine.
     9. Inertial kinetic wheel deceleration.
     10. Hotspot translation.
     11. Real-time telemetry capture (FPS, throughput, jitter, drops).
     12. Dual-mode notification broadcast (GPU Hardware Plane vs. BSPE Software Micro-Tile).
3. **CPE Hardware Cursor Plane Handler (Phase 6):**
   - Implement native hardware cursor ops (`set_cursor_position`, `set_cursor_image`, `set_cursor_visibility`) for:
     - **Intel Native Graphics (`gpu_drv_intel.c`):** Haswell Gen7.5 HD 4400 & Raptor Lake Gen12 UHD 730 via `INTEL_CURACNTR`, `INTEL_CURABASE`, `INTEL_CURAPOS`.
     - **AMD Radeon Native Graphics (`gpu_drv_amd.c`):** GCN/RDNA via `AMD_D1CUR_CONTROL`, `AMD_D1CUR_SURFACE_ADDRESS`, `AMD_D1CUR_POSITION`.
     - **NVIDIA Native Display (`gpu_drv_nvidia.c`):** NV_PRAMDAC via `NV_PRAMDAC_CURSOR_CTRL`, `NV_PRAMDAC_CURSOR_BASE`, `NV_PRAMDAC_CURSOR_POS`.
   - Update `kernel/drivers/input/cursor/cursor_backend.c` and `kernel/graphics/cursor/core/cursor_hal.c` to bind to the active primary GPU hardware cursor plane when available, and fall back to BSPE fast micro-tile when in GOP mode.

---

## 2. MODIFICATION PLAN (FILE-BY-FILE & FUNCTION-BY-FUNCTION)

### A. `kernel/drivers/input/core/aiie_nanoring.h` & `aiie_nanoring.c` (NEW ENGINE MODULE)
* **Purpose:** High-throughput lockless Nano-Ring buffer for Phase 5 IEC.
* **Structures:**
  - `AIIE_Event` struct: `timestamp_ns`, `device_id`, `event_type`, `event_code`, `value`, `flags`.
  - Static circular ring of 512 entries with atomic head and tail pointers.
* **Functions:**
  - `aiie_nanoring_init(void)`: Initialize atomic head/tail to 0.
  - `aiie_nanoring_push(const AIIE_Event* ev)`: Lockless producer push (called from xHCI MSI-X ISR or HIDA).
  - `aiie_nanoring_pop(AIIE_Event* ev)`: Lockless consumer pop (called from pointer engine drain loop).
  - `aiie_nanoring_count(void)`: Live queued event depth.

### B. `kernel/drivers/input/pointer/pointer_motion.c` & `pointer_motion.h`
* **Purpose:** Upgrade to 12-Stage PPE V2 deterministic motion pipeline.
* **Functions to Modify:**
  - `pointer_motion_process(const InputCoreEvent* event)`:
    - Add Stage 2: Spike filtration ($> \pm 8192$ units).
    - Add Stage 9: Kinetic wheel deceleration filter (smooth scrolling physics).
    - Add Stage 11: Telemetry instrumentation (FPS, throughput, jitter).
    - Connect Stage 12 to query `cursor_backend_is_hardware()`: If hardware cursor active, call `cursor_backend_set_position(new_x, new_y)` directly; otherwise call `BSPE_CursorPresenter_FastTileUpdate()`.

### C. `kernel/graphics/gpu/drivers/gpu_drv_intel.c`
* **Purpose:** Implement Phase 6 Intel GPU Hardware Cursor Plane.
* **Functions to Add:**
  - `intel_set_cursor_position(bos_gpu_device_t* dev, int32_t x, int32_t y)`:
    - Pack coordinates into `INTEL_CURAPOS` format: `uint32_t pos = ((y & 0x7FF) << 16) | (x & 0x7FF);`
    - Handle negative bounds with sign bit (Bit 15 for X, Bit 31 for Y).
    - Write to MMIO offset `0x70088` (`INTEL_CURAPOS`).
  - `intel_set_cursor_image(bos_gpu_device_t* dev, const uint32_t* image, uint32_t w, uint32_t h, uint32_t hx, uint32_t hy)`:
    - Copy 64x64 ARGB sprite into dedicated page-aligned cursor buffer in stolen VRAM / GGTT.
    - Write physical cursor base address to `INTEL_CURABASE` (`0x70084`).
    - Configure `INTEL_CURACNTR` (`0x70080`) with `0x27` (64x64 ARGB).
  - `intel_set_cursor_visibility(bos_gpu_device_t* dev, bool visible)`:
    - If visible, write `0x27` to `INTEL_CURACNTR`; if hidden, write `0x00`.
* **Hooks:**
  - Bind `intel_set_cursor_position`, `intel_set_cursor_image`, `intel_set_cursor_visibility` into `g_intel_driver.ops`.

### D. `kernel/graphics/gpu/drivers/gpu_drv_amd.c`
* **Purpose:** Implement Phase 6 AMD GPU Hardware Cursor Plane.
* **Functions to Add:**
  - `amd_set_cursor_position(bos_gpu_device_t* dev, int32_t x, int32_t y)`:
    - Write `(y << 16) | (x & 0xFFFF)` to `AMD_D1CUR_POSITION` (`0x0A08`).
  - `amd_set_cursor_image(bos_gpu_device_t* dev, const uint32_t* image, uint32_t w, uint32_t h, uint32_t hx, uint32_t hy)`:
    - Copy ARGB sprite to dedicated cursor aperture in VRAM.
    - Program `AMD_D1CUR_SURFACE_ADDRESS` (`0x0A04`).
    - Program `AMD_D1CUR_CONTROL` (`0x0A00`) to enable 64x64 32bpp cursor.
  - `amd_set_cursor_visibility(bos_gpu_device_t* dev, bool visible)`:
    - Update `AMD_D1CUR_CONTROL` enable bit.
* **Hooks:**
  - Bind functions into `g_amd_driver.ops`.

### E. `kernel/graphics/gpu/drivers/gpu_drv_nvidia.c`
* **Purpose:** Implement Phase 6 NVIDIA GPU Hardware Cursor Plane.
* **Functions to Add:**
  - `nv_set_cursor_position(bos_gpu_device_t* dev, int32_t x, int32_t y)`:
    - Write to `NV_PRAMDAC_CURSOR_POS` (`0x680808`).
  - `nv_set_cursor_image(bos_gpu_device_t* dev, const uint32_t* image, uint32_t w, uint32_t h, uint32_t hx, uint32_t hy)`:
    - Copy ARGB sprite into VRAM cursor area.
    - Write address to `NV_PRAMDAC_CURSOR_BASE` (`0x680804`).
    - Enable hardware cursor via `NV_PRAMDAC_CURSOR_CTRL` (`0x680800`).
  - `nv_set_cursor_visibility(bos_gpu_device_t* dev, bool visible)`:
    - Toggle enable bit in `NV_PRAMDAC_CURSOR_CTRL`.
* **Hooks:**
  - Bind functions into `g_nvidia_driver.ops`.

### F. `kernel/drivers/input/cursor/cursor_backend.c`
* **Purpose:** Hardware detection and dynamic mode selection.
* **Functions to Modify:**
  - `cursor_backend_init()`:
    - Check `bos_gpu_device_t* gpu = bos_gpu_get_primary()`.
    - If `gpu && gpu->ops && gpu->ops->set_cursor_position && gpu->ops->set_cursor_image`:
      Set `g_active_backend = CURSOR_BACKEND_HARDWARE`.
    - Otherwise, default to `CURSOR_BACKEND_SOFTWARE`.
  - `cursor_backend_is_hardware()`:
    - Return `true` if `g_active_backend == CURSOR_BACKEND_HARDWARE`.

---

## 3. EXPECTED OUTCOMES & METRICS

1. **Hardware Cursor Plane Glide:**
   - On Intel, AMD, NVIDIA, and VMware targets, cursor updates execute in single MMIO writes (< 50 nanoseconds).
   - CPU load for cursor presentation drops to **0.0%**.
   - Cursor motion remains 100% fluid even if desktop compositor is under full stress.
2. **Sub-Millisecond Software Fallback:**
   - In pure UEFI GOP mode (before GPU drivers probe), BSPE software micro-tile presenter continues to render cleanly without regression.
3. **Nano-Ring Integrity:**
   - 0 dropped mouse reports during rapid circles at 1000Hz.
   - Atomic `AIIE_EV_SYN` guarantees zero half-frame tearing.

---

## 4. ROLLBACK & RECOVERY PLAN

- If any hardware cursor register write causes display instability:
  - `cursor_backend_init()` can force `g_active_backend = CURSOR_BACKEND_SOFTWARE`, completely bypassing all GPU MMIO cursor registers.
  - Rollback takes < 5 seconds via `git restore`.
