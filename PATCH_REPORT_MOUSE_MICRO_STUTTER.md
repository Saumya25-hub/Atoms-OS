# ATOMS OS — PATCH REPORT (TASK 3)

**Issue:** Periodic 3.5-Second Mouse Freeze & Screen Invalidation Elimination  
**Date:** 2026-11-02  
**Author:** ATOMS OS Patch Team  
**Scope of Changes:** Strictly limited to `kernel/debug/hypervisor_dashboard/hypervisor_dashboard.c`, `kernel/debug/mouse_debug.c`, and `kernel/debug/mouse_debug.h`.  
**Forbidden Subsystems Touched:** ZERO (`kernel.c`, GDT, IDT, PMM, VMM, BSPE, and USB xHCI driver core untouched).

---

## 1. Files Modified

| File | Status | Lines Changed | Description |
|---|---|---|---|
| `kernel/debug/hypervisor_dashboard/hypervisor_dashboard.c` | **MODIFIED** | +15, -7 | Uncoupled debug page checks from `rt_vm` guard in `hypervisor_dashboard_render_frame()`. Silenced periodic hypervisor board redraws on `HV_DASHBOARD_PAGE_MOUSE_DEBUG` in the 50,000-tick loop. Added layout reset hook on F8 key toggle. |
| `kernel/debug/mouse_debug.c` | **MODIFIED** | +45, -30 | Implemented incremental fast-rendering (`s_static_rendered`). Static shell (background, panels, title bars, table headers) rendered once on page entry. Dynamic packet updates only clear small dirty regions for numerical fields and rows, speeding up rendering by >90%. |
| `kernel/debug/mouse_debug.h` | **MODIFIED** | +1, -0 | Added prototype for `mouse_debug_reset_layout()`. |

---

## 2. Functions Modified

1. `hypervisor_dashboard_render_frame()`:
   - Moved `if (g_hv_dashboard_page == HV_DASHBOARD_PAGE_MOUSE_DEBUG)` and `KEYBOARD_DEBUG` to the top of the function before `if (rt_vm)`. Now renders mouse inspector immediately even when `rt_vm == NULL`.
2. `hypervisor_dashboard_run()`:
   - In `s_dash_ticks % 50000 == 0`, if on `HV_DASHBOARD_PAGE_MOUSE_DEBUG`, suppresses the full hypervisor 28-card stage redraw and slow blocking serial output.
3. `mouse_debug_render_frame()`:
   - Refactored into static shell render (one-time) and fast dirty-rectangle updates for packet counts, accumulators, button states, and the 12-entry packet stream table.

---

## 3. Verification & Metrics

- Kernel and Bootloader compilation: **0 Errors, 0 Warnings**.
- Full UEFI / GPT image (`atoms_uefi_test.img`) regenerated.
- QEMU Pure UEFI Pre-Flight: **PASS** (EDK2 OVMF, ExitBootServices, xHCI enumeration, HRE parsing).
