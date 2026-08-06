# BOS Cursor Engine (BCE) V1.0 — Architecture & Developer Guide

> **Note for AI Coding Assistants & Developers**: This document details the complete end-to-end architecture, data flow, decoder implementations, hardware GPU upload path, and 1000Hz IRQ animation engine of the **BOS Cursor Engine (BCE V1.0)** in **ATOMS OS**.

---

## 📍 Quick File Reference Map

- **Master Engine API & State**: [bos_cursor.h](file:///d:/Signatures_OS/kernel/graphics/cursor/include/bos_cursor.h), [bos_cursor.c](file:///d:/Signatures_OS/kernel/graphics/cursor/core/bos_cursor.c)
- **Windows `.cur` Decoder**: [bos_cur_loader.h](file:///d:/Signatures_OS/kernel/graphics/cursor/include/bos_cur_loader.h), [cur_loader.c](file:///d:/Signatures_OS/kernel/graphics/cursor/core/cur_loader.c)
- **Windows RIFF `.ani` Decoder**: [bos_ani_loader.h](file:///d:/Signatures_OS/kernel/graphics/cursor/include/bos_ani_loader.h), [ani_loader.c](file:///d:/Signatures_OS/kernel/graphics/cursor/core/ani_loader.c)
- **Embedded Win11 Theme Assets**: [w11_cursor_assets.h](file:///d:/Signatures_OS/kernel/graphics/cursor/include/w11_cursor_assets.h)
- **GPU Hardware HAL & Upload**: [cursor_hal.c](file:///d:/Signatures_OS/kernel/graphics/cursor/core/cursor_hal.c), [gpu_drv_vmware.c](file:///d:/Signatures_OS/kernel/graphics/gpu/drivers/gpu_drv_vmware.c#L420-L460)
- **1000Hz PIT IRQ 0 Timer Integration**: [timer.c](file:///d:/Signatures_OS/kernel/core/timer/src/timer.c#L20-L30)
- **Compositor Dual-Sync Presenter Bridge**: [bspe_cursor_present.c](file:///d:/Signatures_OS/kernel/graphics/BSPE/Cursor/bspe_cursor_present.c#L155-L168)
- **App Launching Integration**: [horse_engine.c](file:///d:/Signatures_OS/kernel/engine/horse_engine.c#L170-L195), [explorer.c](file:///d:/Signatures_OS/kernel/shell/apps/explorer.c#L587-L595), [desktop_shell.c](file:///d:/Signatures_OS/kernel/shell/desktop_shell/desktop_shell.c#L620-L625)
- **Automated Certification Tests**: [cursor_tests.c](file:///d:/Signatures_OS/kernel/graphics/cursor/tests/cursor_tests.c)
- **Detailed Architecture Specification**: [docs/architecture/bos_cursor_engine_v1.md](file:///d:/Signatures_OS/docs/architecture/bos_cursor_engine_v1.md)

---

## 🛠️ Summary of the Hardware/Software Architecture

### 1. DIB ARGB `.cur` & RIFF `.ani` Decoders
- `.cur` files are decoded in `cur_loader.c` by parsing DIB `BITMAPINFOHEADER` headers, converting bottom-up BGRA pixels to top-down 32-bit ARGB (`0xAARRGGBB`), and resolving 1-bit AND bitplane transparency masks.
- `.ani` files are parsed in `ani_loader.c` by traversing RIFF `ACON` structures, `anih` animation headers, `rate` jiffy tables, and `'fram'` list icon chunks to extract all 37 frames into `bce_cursor_t->frames[]`.

### 2. VMware SVGA II Hardware Cursor Upload
- `gpu_drv_vmware.c` implements `vmware_set_cursor_image()` using Command 19 (`SVGA_CMD_DEFINE_CURSOR`).
- Color pixels and AND mask bitplanes are reserved in the VMware Command FIFO via `fifo_reserve()`, committed via `fifo_commit()`, and flushed via `gpu_svga_write_reg(svga, SVGA_REG_SYNC, 1)`.

### 3. 1000Hz Hardware PIT IRQ 0 Timer Animation Loop
- `timer.c` calls `bos_cursor_tick()` on every 1ms hardware timer IRQ 0.
- `bos_cursor_tick()` calculates `elapsed_ms` using `timer_get_ticks()`.
- Calculates 25 FPS frame index: `frame_idx = (elapsed_ms / 40) % frame_count`.
- `bos_cursor_hal_upload_frame()` uploads the active frame to GPU hardware registers AND calls `BSPE_CursorPresenter_UpdateBitmap()` to update the software compositor buffer `s_bitmap` simultaneously.

### 4. App Launching State Machine
- When an icon is double-clicked or `horse_launch()` / `Explorer_LaunchPath()` is called:
  - `bos_cursor_set_active_type(BCE_CURSOR_APPSTARTING)` sets active cursor to `appstarting.ani` and starts a **3.5-second timed loading window** (`g_anim_timeout_ms = 3500`).
  - The 37-frame animated spinner rotates smoothly at 25 FPS on IRQ 0.
  - When the 3.5-second window expires, `bos_cursor_tick()` automatically reverts active cursor back to `arrow.cur` (`BCE_CURSOR_ARROW`).

---

## 🚢 Git Version Control Tag

- **Release Tag**: `mouse-engine-v2-ani-cur`
- **GitHub Repository**: [Saumya25-hub/SignaturesOS](https://github.com/Saumya25-hub/SignaturesOS)
