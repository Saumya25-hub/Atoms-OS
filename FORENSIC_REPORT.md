# ATOMS OS — MASTER FORENSIC REPORT (v3)
## Phase: Dashboard Input Lag, Choppy Heartbeat Spinner, and Full-Panel Flicker Elimination
### Target Hardware: ASUS PRIME B760M-K (Intel Core i3-14100F, Raptor Lake, 4C/8T, LGA1700)
### Governing Rule: RULE 0 — Mandatory Phase Isolation (Investigate ➔ Plan ➔ Patch ➔ Certify)

**Date:** 2026-10-23  
**Status:** TASK 1 COMPLETE (Forensic Audit & Telemetry Measurement Complete — NO CODE MODIFIED)  
**Target Hardware Profile:**  
- Motherboard: ASUS PRIME B760M-K (Intel B760 Chipset, LGA1700, MAC `A0:AD:9F:C5:81:27`)  
- CPU: Intel Core i3-14100F (Raptor Lake, 4 P-cores, 8 logical threads, 3.5 - 4.7 GHz)  
- RAM: 16 GB DDR5  
- Display Output: UEFI GOP 32-bit Framebuffer (Physical VRAM mapped over PCIe)  
- Firmware Mode: Native UEFI x86_64 Mode  

---

## 1. Executive Summary & Problem Definition

During bare-metal execution on the primary target (Intel Core i3-14100F on ASUS PRIME B760M-K), the user observed three severe UI regressions:
1. **Input Lag / Missed Keypresses**: Tapping F1, F2, F3 to switch dashboard pages does not switch pages unless the key is physically held down.
2. **Choppy / Stuttering Heartbeat Spinner**: The `| / - \` spinner advances irregularly and feels slow/laggy.
3. **Full-Panel Refresh Flicker**: On every refresh, the entire dashboard panel momentarily disappears (wipes to black) and visibly redraws from top to bottom.

In accordance with Step 1 and Step 2 of the Engineering Protocol, precision RDTSC and tick telemetry instrumentation was inserted into the live kernel to measure exact loop timings, execution breakdowns, hardware registers, and memory caching attributes before modifying any functional logic.

---

## 2. Step 1 — Forensic Measurements (Baseline Pre-Fix Numbers)

Telemetry probes recorded live via serial COM1 (`[UI_PERF_BEFORE]` stream) during active kernel execution:

| Metric | Measured Baseline (Pre-Fix) | Target / Requirement | Unit / Notes |
| :--- | :--- | :--- | :--- |
| **Dashboard Loop Period (Min)** | **109 ms** | $\le 20$ ms | Minimum interval between frames |
| **Dashboard Loop Period (Avg)** | **180 – 185 ms** | $\le 33$ ms | Effective frame rate $\approx 5.4$ FPS |
| **Dashboard Loop Period (Max)** | **246 ms** | $\le 45$ ms | Severe latency spike |
| **Time Spent in Sleep** | **102 – 111 ms** | Decoupled ($\le 2$ ms input, 25 ms UI) | Hardcoded `scheduler_sleep(100)` in `system_telemetry_thread` |
| **Time Spent in Render** | **76 – 81 ms** | $\le 5$ ms | Direct unaccelerated PCIe writes to front VRAM |
| **Time Spent in VM50 (50 exits)** | **0 – 38 ms** | $\le 10$ ms | 0 ms when guest idle, up to 38 ms under load |
| **Full-Screen Clear Time (`abde_fill_rect`)** | **44 – 74 ms** | **0 ms** (Eliminate full-screen wipe) | Clearing 2560x1600 (16.38 MB) directly to VRAM |
| **VRAM Write Bandwidth** | **211 – 355 MB/s** | N/A (RAM backbuffer $> 15,000$ MB/s) | Unbuffered uncached/write-through PCIe bottleneck |
| **Per-Frame Bytes Written to VRAM** | **16,384,000 bytes (16.4 MB)** | $\le 0.5$ MB (Dirty updates only) | Entire screen wiped black before text is drawn |
| **xHCI Polling Frequency** | **5.4 – 6.1 Hz** | $500 – 1000$ Hz (1–2 ms fast path) | Polled only once per 185 ms dashboard loop |
| **xHCI Events Drained per Call** | **1 event** (0–2 burst) | All pending TRBs drained | Starved TRB pipeline |
| **Key Latency (xHCI $\to$ Page Switch)**| **185 – 240 ms** (or dropped) | $\le 50$ ms | Short 20–40 ms taps missed completely |
| **Framebuffer Page Table Mapping** | **PDE `0x00000000800000EB`** | PAT Index 1 = WC on ALL CPUs | Present, Writable, Huge, PWT=1 (PAT Index 1) |
| **PAT MSR (0x277) on AP Cores** | **UNINITIALIZED (Default WT)** | Symmetrically initialized on APs | APs default to Intel WT (Write-Through) |

---

## 3. Step 2 — Verification of Hypotheses

### Hypothesis H1: Input is polled only inside the ~150–220ms dashboard loop.
- **Verdict: CONFIRMED.**
- **Evidence:** `xhci_poll()` was called exclusively from `hypervisor_dashboard_step()` in `hypervisor_dashboard.c:2643` and `hv_poll_developer_toggle_keys():1266`. `hypervisor_dashboard_step()` is invoked only from `system_telemetry_thread()` in `kernel/kernel.c:114`, which sleeps for `scheduler_sleep(100)` on every iteration. With 80 ms render time + 100 ms sleep, the period was measured at 180–185 ms. Input was completely blind for $> 175$ ms out of every 180 ms.

### Hypothesis H2: Key handling reads "current key state" and drops queued press/release events; TRB pipeline is starved.
- **Verdict: CONFIRMED.**
- **Evidence:** In `kernel/drivers/usb/host/xhci/xhci_transfer.c:465`, only ONE interrupt transfer TRB is queued at any time (`xhci_interrupt_in_transfer`). Once a keyboard packet is received, the endpoint has ZERO TRBs queued until `xhci_poll()` runs and requeues a TRB. If the user taps a key for 25–40 ms, the key-down and key-up occur while software is asleep in `scheduler_sleep(100)`. Because the pipeline was starved, the endpoint NAKed or dropped the event. When holding the key down, auto-repeat continued firing packets over 200–500 ms, ensuring at least one reached the polled window.

### Hypothesis H3: Spinner advances once per dashboard loop, not by time.
- **Verdict: CONFIRMED.**
- **Evidence:** In `hypervisor_dashboard.c:2667-2668`:
  `s_dash_ticks++; char spin_char = s_spin[s_dash_ticks % 4];`
  Because `s_dash_ticks` incremented once per ~185 ms loop iteration, the spinner rotated at only ~1.3 rotations per second, with severe stutter whenever render or VM steps took longer.

### Hypothesis H4: Framebuffer caching is degraded on AP cores.
- **Verdict: CONFIRMED.**
- **Evidence:** Although `vmm.c:20-38` defines `vmm_init_pat()` to configure PA1 as Write-Combining (`0x01`), `vmm_init_pat()` was static and invoked ONLY on the BSP in `vmm_enable()`. The IA32_PAT MSR (`0x277`) is an architectural per-core MSR. All AP cores (1 through 7) booted with Intel's default reset value (`0x0007040600070406ULL`), where PA1 is **0x04 = Write-Through (WT)**! Any dashboard or telemetry rendering executing on AP cores crawled in WT mode at 211–355 MB/s, heavily stalling execution pipelines.

### Hypothesis H5: Each frame clears the whole screen to black before drawing.
- **Verdict: CONFIRMED.**
- **Evidence:** In `hypervisor_dashboard.c:2789` and `3224`, `abde_fill_rect(0, 0, screen_w, screen_h, c_bg)` was executed at the top of every single frame, writing 16.38 MB directly to physical VRAM. Because this was written directly to the scanned-out front buffer without synchronization or backbuffering, the monitor scanned the dark blank rectangle for 44–74 ms on every frame before the cards and text were redrawn, causing continuous, nauseating full-panel flicker.

---

## 4. Subsystems & Files Involved

1. `kernel/kernel.c`:
   - Contains `system_telemetry_thread()` which throttles the UI loop with `scheduler_sleep(100)`.
   - Host for the new dedicated fast input polling thread.
2. `kernel/core/memory/vmm/src/vmm.c` & `kernel/core/memory/vmm/include/vmm.h`:
   - Houses `vmm_init_pat()`, which needs to be exported and callable by all CPUs.
3. `arch/x86_64/smp/smp.c`:
   - Houses `ap_main()`, which brings up AP cores 1..7 without calling `vmm_init_pat()`.
4. `kernel/drivers/usb/class/usb_hid.c`:
   - Contains synchronous `display_print` calls over 115200 baud UART on keypresses that add millisecond delays to input dispatch.
5. `kernel/debug/hypervisor_dashboard/hypervisor_dashboard.c`:
   - Direct front-buffer rendering, full-screen canvas clear on every frame, loop-based spinner, coupled VM-exit budget, and key toggle processing.
6. `kernel/debug/abde/abde_renderer.c`:
   - Houses `abde_fill_rect`, `abde_render_string`, and `abde_render_char` which write to `g_abde.framebuffer`.

---

## 5. Risk Analysis & Safety Invariants

- **Risk 1: Multi-Core PAT Desynchronization**: If PAT MSR is not programmed identically across all cores, page table access behavior diverges between cores.
  - *Mitigation*: Invoke `vmm_init_pat()` on BSP and on every AP in `ap_main()` immediately after SSE/MSR initialization.
- **Risk 2: Back Buffer Memory Footprint & Allocation Crash**: Allocating 16 MB dynamically at runtime could fail or corrupt heap.
  - *Mitigation*: Use a statically allocated BSS back buffer (`static uint32_t s_dash_backbuffer[2560 * 1600] __attribute__((aligned(64)))`), which costs 0 bytes in the EFI binary and is mapped at boot.
- **Risk 3: Panic / Fault Screen Invisibility**: If a kernel panic occurs while `g_abde.framebuffer` points to the RAM back buffer, the panic screen might never reach the monitor.
  - *Mitigation*: The panic/fault path must immediately restore direct rendering to physical VRAM (`g_abde.framebuffer = g_abde.physical_framebuffer`) before rendering exception text, ensuring fail-safe visibility on safe stacks.
- **Risk 4: Guest Virtualization / FreeBSD Gate Regression**: Decoupling the hypervisor step must not starve the VM.
  - *Mitigation*: Maintain `atoms_hypervisor_runtime_step()` inside the UI cadence with an execution time budget or run it on its dedicated cadence.

---

## 6. Suspected Fix Outline (NO CODE)

1. **Fix A (Fast Input Path)**: Create a dedicated fast input polling kernel thread running at 1–2 ms cadence. Drain all pending xHCI event TRBs per call. Push key events into the existing lock-free/irq-safe ring buffer. Strip slow UART prints from the input path.
2. **Fix B (Symmetric PAT WC)**: Export `vmm_init_pat()` and call it in `ap_main()` on every AP core so that all 8 cores treat PA1 as Write-Combining.
3. **Fix C (RAM Back Buffer)**: Route dashboard rendering into a static RAM back buffer, then blit to physical VRAM in a single fast 64-bit pass.
4. **Fix D (Dirty Rectangles / Static Caching)**: Render the static layout (headers, borders, card backgrounds) once on page transition; per frame, redraw only modified telemetry cells and blit only changed regions. Eliminate full-screen black clear per frame.
5. **Fix E (Time-Based Spinner & UI Decoupling)**: Derive spinner rotation from timer ticks (`timer_get_ticks() / 125 % 4`). Reduce UI thread sleep to 25–30 ms to achieve $\le 33$ ms loop period (30 FPS).
6. **Fix F (Safe Panic Fallback)**: In exception and panic handlers, bypass back buffering and render directly to physical VRAM.
