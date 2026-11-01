# ATOMS OS — FORENSIC INVESTIGATION REPORT (TASK 1)

**Issue:** Periodic 3-4 Second Mouse Movement Freeze & Screen Flash ("REFREASH THODA SCREEN")  
**Target Hardware:** ASUS PRIME B760M-K (Intel Core i3-14100F, Raptor Lake Refresh, xHCI)  
**Date:** 2026-11-02  
**Author:** ATOMS OS Forensic Group  
**Rule 0 Compliance:** Investigation & Evidence Only (NO CODE MODIFICATIONS)  

---

## 1. Physical Hardware Observation

During live bare-metal hardware testing of the **HRE (Universal HID 1.11 Bytecode Parser)**:
1. **Core Mouse Tracking Works:** Physical mouse movement, coordinate accumulation, buttons, and scroll wheel operate with high accuracy.
2. **Periodic Interruption:** Every 3 to 4 seconds, cursor movement freezes for approximately 500ms–1000ms while the screen undergoes a slight refresh/flicker, after which movement instantly resumes butter-smooth for another 3–4 seconds:
   $$\text{Move (Smooth 3-4s)} \longrightarrow \text{Stuck / Screen Refresh (1s)} \longrightarrow \text{Move (Smooth 3-4s)}$$

---

## 2. Root Cause Analysis & Forensic Evidence

### Evidence A: The 3.5-Second Heartbeat Periodic Redraw Loop
In `kernel/debug/hypervisor_dashboard/hypervisor_dashboard.c:2860-2898`:
```c
s_dash_ticks++;
if ((s_dash_ticks % 50000) == 0) {
    char spin_char = s_spin[(s_dash_ticks / 50000) % 4];

    if (g_autopsy_engine.has_post_failure) {
        ...
    } else if (rt_vm) {
        ...
    } else {
        hypervisor_dashboard_render_frame();
        char spinner_msg[64];
        hv_format_heartbeat(spinner_msg, ...);
        com1_puts(spinner_msg);
        com1_puts("\r\n");
    }
}
```

* **Observation:** On the Intel Core i3-14100F running in the diagnostic loop, `s_dash_ticks % 50000 == 0` evaluates to true **every ~3.5 seconds**.
* **The Fault:** Because `rt_vm == NULL` (the micro-hypervisor halted at Stage 5 VM_CREATE due to RAM reservation limit), execution unconditionally falls into the `else` block and invokes `hypervisor_dashboard_render_frame()`.

---

### Evidence B: Page Condition Inversion in `hypervisor_dashboard_render_frame()`
In `kernel/debug/hypervisor_dashboard/hypervisor_dashboard.c:420-456`:
```c
/* Check if persistent runtime VM exists */
VirtualMachine *rt_vm = atoms_hypervisor_get_runtime_vm();
if (rt_vm) {
    if (g_hv_dashboard_page == HV_DASHBOARD_PAGE_MOUSE_DEBUG) {
        extern void mouse_debug_render_frame(void);
        mouse_debug_render_frame();
        return;
    }
    ...
}

/* Color Palette */
uint32_t c_bg = 0xFF0D1117;
// RENDERS ALL 28 HYPERVISOR STAGE CARDS ACROSS THE ENTIRE SCREEN!
```

* **The Fault:** The check `if (g_hv_dashboard_page == HV_DASHBOARD_PAGE_MOUSE_DEBUG)` is mistakenly placed **INSIDE** `if (rt_vm)`.
* When `rt_vm == NULL`, the function ignores that the operator is on the Mouse Debug page. It clears the entire 2560x1600 framebuffer (`abde_fill_rect`) and redraws the complete 28-stage Hypervisor board.
* It then executes `com1_puts()` to stream telemetry across the slow UART serial port, blocking the CPU for hundreds of milliseconds. During this time, `xhci_poll()` is completely starved.

---

### Evidence C: The Screen Fighting & Stutter Cycle
1. At $T = 0.0\text{s}$: Operator moves mouse. `mouse_debug_render_frame()` draws the Mouse Inspector. Motion is smooth.
2. At $T = 3.5\text{s}$: `s_dash_ticks % 50000 == 0` fires. `hypervisor_dashboard_render_frame()` takes over, wipes the framebuffer, draws the hypervisor stage cards, and blocks on UART.
   * **Result:** The screen flickers with a different layout, and mouse tracking stops completely.
3. At $T = 4.2\text{s}$: Serial dump finishes, loop returns to `hv_poll_developer_toggle_keys()`, `xhci_poll()` runs, mouse packet arrives.
4. `mouse_debug_render_frame()` instantly clears the framebuffer again and redraws the Mouse Inspector.
5. Motion resumes smoothly until $T = 7.0\text{s}$ when the next heartbeat fires.

---

## 3. Secondary Bottleneck: Full Framebuffer Clear Per Packet
In `kernel/debug/mouse_debug.c:108`:
```c
/* 1. Clear Framebuffer background */
abde_fill_rect(0, 0, 1280, 800, bg_color);
```
Every incoming mouse packet currently wipes and redraws the entire 1280x800 background, title bar, and table header over uncached PCIe VRAM, rather than only updating the changed rows and accumulator values.

---

## 4. Suspected Fix Strategy (NO CODE APPLIED)

1. **Decouple Page Checks from `rt_vm`:** Move `if (g_hv_dashboard_page == HV_DASHBOARD_PAGE_MOUSE_DEBUG)` to the top of `hypervisor_dashboard_render_frame()` so it applies regardless of `rt_vm` state.
2. **Silence Periodic Redraw on Debug Pages:** In the 50,000-tick loop, if `g_hv_dashboard_page == HV_DASHBOARD_PAGE_MOUSE_DEBUG`, only update the bottom heartbeat indicator or spinner without redrawing the hypervisor stage cards or stalling on blocking serial dumps.
3. **Incremental Mouse Telemetry Updates:** Cache static background elements and only redraw dynamic text/packet rows, reducing PCIe VRAM bandwidth by >90%.
