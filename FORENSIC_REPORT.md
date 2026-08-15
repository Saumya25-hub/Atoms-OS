# 🔬 FORENSIC INVESTIGATION REPORT: CURSOR FLICKER ELIMINATION & USB HOT-PATH PROFILING
**Subsystem:** ATOMS OS Input & Compositor Subsystems (`xhci.c`, `rook_render.c`, `rook_core.c`, `pointer_velocity.c`)  
**Investigating Agent:** Antigravity / ARYA Core Forensic  
**Date:** 2026-08-16  
**Status:** TASK 1 COMPLETE (Forensic Phase — NO CODE)

---

## 1. Executive Summary & Forensic Root Cause
1. **Root Cause 1 (Mouse Latency): Hot-Path Debug Printing in `xhci_poll()`:**
   - In `kernel/drivers/usb/host/xhci/xhci.c` (lines 376–382), `display_print("[XHCI EVENT] XFER Slot=...")` was executing inside the hot transfer event handler on **every single USB mouse packet**.
   - Outputting 35 characters through VGA/UART at 115200 baud blocked the CPU core for $\approx 3.5\text{ms}$ per packet, saturating the transfer queue and causing massive input lag.
2. **Root Cause 2 (Cursor Blink / Flicker): Asynchronous VRAM Overwrite Tearing:**
   - `rook_render_flush()` copied the Backbuffer (which lacked the cursor) to physical VRAM, temporarily wiping the cursor before `rook_cursor_force_redraw()` repainted it.
   - When the 60Hz display panel scanned out VRAM during this sub-millisecond gap, the frame rendered without a cursor, producing visible blinking.

---

## 2. Real OS Linux DRM / Windows DWM Architecture
1. **Eliminate All UART/Display I/O from Interrupt & Polling Hot Paths:**
   - Strip all `display_print` statements from `xhci_poll()` and `usb_hid.c`.
2. **Atomic Backbuffer Cursor Compositing:**
   - Compose the mouse cursor directly into `target_buf` (Backbuffer) before flushing dirty regions to physical GOP VRAM.
   - On cursor motion: Restore clean background from `s_wallpaper_canvas` at old rect, composite cursor at new rect in Backbuffer, and flush only the $40\times40$ dirty rectangles ($\approx 2\mu\text{s}$).

---

## 3. Files Involved
* `kernel/drivers/usb/host/xhci/xhci.c`: Remove hot-path `display_print` debug calls from `xhci_poll()`.
* `kernel/shell/rook/src/rook_render.c`: Implement atomic Backbuffer cursor compositing.
* `kernel/shell/rook/src/rook_core.c`: Synchronize 1000Hz fast-path mouse blitting.
