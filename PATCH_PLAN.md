# ATOMS OS — MASTER ARCHITECTURE PATCH PLAN (v3)
## Phase: Dashboard Input Lag, Choppy Heartbeat Spinner, and Full-Panel Flicker Elimination
### Target Hardware: ASUS PRIME B760M-K (Intel Core i3-14100F, Raptor Lake, 4C/8T, LGA1700)
### Governing Rule: RULE 0 — Mandatory Phase Isolation (Investigate ➔ Plan ➔ Patch ➔ Certify)

**Date:** 2026-10-23  
**Status:** TASK 2 COMPLETE (Architectural Plan Formulated — NO CODE MODIFIED)  
**Input Document:** [FORENSIC_REPORT.md](file:///d:/Signatures_OS/FORENSIC_REPORT.md)  
**Safety Backup Tag:** `backup-pre-ui-perf` (commit `4b46ee6`)  
**Active Working Branch:** `feature/ui-framebuffer-input-perf`  

---

### 1. Whitelisted Files for Modification

Under strict RULE 0, modifications are permitted ONLY to the following files:

1. `kernel/core/memory/vmm/src/vmm.c`: Export `vmm_init_pat()` as non-static.
2. `kernel/core/memory/vmm/include/vmm.h`: Declare `void vmm_init_pat(void);`.
3. `arch/x86_64/smp/smp.c`: Invoke `vmm_init_pat()` in `ap_main()` on every AP.
4. `kernel/drivers/usb/class/usb_hid.c`: Silence slow serial/LAN prints in the critical key arrival path.
5. `kernel/kernel.c`:
   - Add dedicated high-frequency input polling thread (`system_input_poll_thread`) at 1–2 ms cadence.
   - Adjust `system_telemetry_thread()` sleep from 100 ms to 25–30 ms to achieve $\le 33$ ms loop period (30 FPS).
6. `kernel/debug/hypervisor_dashboard/hypervisor_dashboard.c`:
   - Back buffer management: allocate static RAM back buffer, route dashboard drawing to RAM back buffer, and blit to physical VRAM.
   - Dirty rects / static frame caching: draw background, cards, and static labels once on page switch; per frame redraw only updated metrics and spinner cell.
   - Time-based spinner: advance spinner character based on timer ticks (`timer_get_ticks() / 125 % 4`).
   - Safe panic path: ensure direct physical VRAM rendering on panic or exception.

No scheduler, memory management allocator, or SSM algorithm files may be touched.

---

### 2. Detailed Component Specifications

#### A. Input Fast Path & Zero-Allocation Ring Buffering
- **What to modify**:
  - In `kernel.c`, spawn a dedicated kernel task `system_input_poll_thread` with priority 28. It will loop continuously, calling `xhci_poll()` and sleeping for 2 ms via `scheduler_sleep(2)`.
  - In `usb_hid.c`, remove `display_print` calls from the hot `usb_hid_report_received` path to eliminate 3–5 ms UART stalling per key.
- **Why**: `xhci_poll()` was previously called only once every ~185 ms inside the telemetry loop. Moving it to its own 2 ms thread guarantees that USB HID transfer events and TRB completions are processed within 1–2 ms of hardware generation, and TRBs are immediately requeued so the hardware endpoint never starves.
- **Expected Result**: Key latency drops from 185–240 ms down to $\le 20$ ms. 100 quick taps of F1/F2/F3 are reliably captured without needing to hold down keys.
- **Risk**: Potential thread scheduling contention. Mitigated by setting task priority to 28 (high kernel priority) and low CPU utilization per poll.

#### B. Symmetric Write-Combining (WC) PAT MSR Setup
- **What to modify**:
  - In `vmm.h` and `vmm.c`, export `vmm_init_pat(void)`.
  - In `smp.c`, call `vmm_init_pat()` inside `ap_main()` on every AP core before starting the multicore scheduler.
- **Why**: PAT MSR (`0x277`) is per-core. APs currently default to Intel reset value where PA1 is Write-Through (WT). Calling `vmm_init_pat()` ensures all 8 logical cores map the framebuffer as Write-Combining (WC).
- **Expected Result**: Full-screen VRAM write throughput increases dramatically; pipeline stalls on VRAM stores are eliminated.
- **Risk**: Very low. `vmm_init_pat()` only writes MSR 0x277 with the standard Linux/Windows PAT layout.

#### C. Static RAM Back Buffer & Front-Buffer Blitting
- **What to modify**:
  - In `hypervisor_dashboard.c`, declare a static aligned RAM back buffer:
    `static uint32_t s_dash_backbuffer[2560 * 1600] __attribute__((aligned(64)));`
  - Temporarily redirect `g_abde.framebuffer` to point to `s_dash_backbuffer` during dashboard rendering passes.
  - Implement a fast 64-bit blitter to copy modified rectangle regions from `s_dash_backbuffer` to the physical front framebuffer (`g_abde.physical_framebuffer` or `boot_info->vbe_framebuffer`).
- **Why**: Direct rendering to unaccelerated PCIe VRAM is slow (355 MB/s) and visible to display scanout (causing flicker). Rendering to L1/L2-cached RAM takes $< 1$ ms, and blitting to Write-Combining VRAM using 64-bit writes eliminates tearing and flicker.
- **Expected Result**: Render time drops from 76–81 ms to $< 4$ ms.
- **Risk**: Statically allocating 16 MB in BSS costs 0 bytes in the `.efi` image and resides safely in system RAM.

#### D. Dirty Rectangles & Static Frame Caching
- **What to modify**:
  - In `hypervisor_dashboard.c`, track `s_last_rendered_page`.
  - When `g_hv_dashboard_page != s_last_rendered_page`:
    Perform a complete canvas clear and draw all static headers, borders, and card frames into the back buffer, copy to front buffer, and update `s_last_rendered_page`.
  - On subsequent frames of the same page:
    Skip the full-screen background wipe (`abde_fill_rect(0, 0, screen_w, screen_h)`). Only overwrite the dynamic text/number regions and the spinner cell, and blit only those dirty bounds to the front buffer.
- **Why**: Eliminates writing 16.38 MB of dark canvas every 180 ms. Only a few kilobytes of pixels are modified and pushed per frame.
- **Expected Result**: Zero full-panel flicker; rock-solid stable display.
- **Risk**: Ensuring all dynamic metrics properly overwrite their previous values (e.g., using background color fill on the label bounding box).

#### E. Time-Based Spinner & UI Frame Decoupling
- **What to modify**:
  - In `hypervisor_dashboard.c`, derive the spinner index directly from physical timer ticks:
    `char spin_char = s_spin[(timer_get_ticks() / 125) % 4];`
  - In `kernel.c`, reduce `system_telemetry_thread()` sleep from 100 ms to 25–30 ms.
  - In `hypervisor_dashboard.c`, cap hypervisor step execution time to 10 ms per UI frame if needed.
- **Why**: Spinner rotation becomes independent of loop rate or momentary rendering delays, rotating at exactly 2 revolutions per second. Telemetry thread achieves a 30–33 ms period (30 FPS).
- **Expected Result**: Dashboard loop period p95 $\le 33$ ms; spinner is fluid and smooth.
- **Risk**: Increased CPU usage for telemetry thread. Mitigated by $< 4$ ms render time.

#### F. Fail-Safe Direct Panic/Fault Rendering
- **What to modify**:
  - In `hypervisor_dashboard.c`, ensure `g_abde.framebuffer` is restored to `boot_info->vbe_framebuffer` on exit or fault.
  - In exception handlers and `diag_panic_reason()`, verify that `g_abde.framebuffer` points to the physical VRAM front buffer before drawing any fault diagnostics.
- **Why**: If a kernel fault occurs, the fault screen must be drawn directly to physical VRAM so that it is visible even if the scheduler or back buffer copy never runs again.
- **Expected Result**: 100% reliable crash diagnostics and screenshot captures.
- **Risk**: None. Preserves existing panic capture behavior.

---

### 3. Rollback Plan

If any regression occurs during testing:
1. Revert to the known-good baseline tag:
   ```bash
   git checkout feature/ui-framebuffer-input-perf
   git reset --hard backup-pre-ui-perf
   ```
2. Rebuild: `powershell -ExecutionPolicy Bypass -File build.ps1`
3. Verify QEMU UEFI 8-Core SMP and FreeBSD gates.
