# EMERGENCY REPRODUCTION MATRIX — ATOMS OS

**Document ID:** `EMERGENCY_REPRODUCTION_MATRIX.md`  
**Classification:** Reproduction Conditions & Environmental Verification Matrix  

---

## 1. Multi-App Workload Reproduction Matrix

| Workload Configuration | Active Windows / Controls | Composition Time per Frame | QEMU (TCG) | VMware Workstation | Physical Haswell H81 |
|---|---|---|---|---|---|
| **0 Apps (Idle Desktop)** | Desktop (1) + Taskbar (1) | ~1.2 ms | PASS | PASS | PASS |
| **1 App (Calculator Only)** | Calculator (19 controls) | ~4.5 ms | PASS | PASS | PASS |
| **1 App (Explorer Only)** | Explorer (1) + Toolbar | ~6.8 ms | PASS | PASS | PASS |
| **2 Apps (Explorer + Terminal)** | Explorer + Terminal (4) | ~18.5 ms | PASS | PASS | PASS |
| **3 Apps (Explorer + Terminal + Calculator)** | Explorer + Terminal + Calculator (25 nodes) | **48.2 ms - 58.0 ms** | PASS (Stutter) | **FAIL (vCPU Shutdown State)** | **FAIL (Hard Reset / Power-off)** |
| **4 Apps (Explorer + Terminal + Calculator + Settings)** | 4 windows (38 nodes) | **65.0 ms - 78.0 ms** | Stutter | **FAIL (vCPU Shutdown State)** | **FAIL (Hard Reset / Power-off)** |

---

## 2. Calculator Interaction Breakdown (under 3-App Workload)

| Operation / Step | Action | State Mutation | Triggered Path | Fault Trigger Risk |
|---|---|---|---|---|
| **Step 1: Launch Explorer** | Click Explorer dock icon | Window created | BWE Window allocation | Low |
| **Step 2: Launch Terminal** | Click Terminal dock icon | Window created | Canvas + Textbox allocation | Low |
| **Step 3: Launch Calculator** | Click Calculator dock icon | Window created | 19 control hierarchy allocated | Low |
| **Step 4: Click '2'** | Mouse Down on button '2' | `ctx->display_text = "2"` | `BWE_InvalidateWindow` $\to$ In-ISR `BWE_ComposeFrame` | **HIGH (First multi-window composition)** |
| **Step 5: Click '+'** | Mouse Down on button '+' | `ctx->op = '+'` | State change | Moderate |
| **Step 6: Click '2'** | Mouse Down on button '2' | `ctx->display_text = "2"` | `BWE_InvalidateWindow` $\to$ In-ISR `BWE_ComposeFrame` | **HIGH** |
| **Step 7: Click '='** | Mouse Down on button '=' | `ctx->display_text = "4"` | `update_calc_display` $\to$ `BWE_InvalidateWindow` $\to$ In-ISR `BWE_ComposeFrame` | **CRITICAL (Peak dirty rects & overdraw)** |

---

## 3. Minimum Condition Required to Trigger the Crash
1. Concurrent execution of at least 3 graphical windows (e.g. Explorer + Terminal + Calculator).
2. Direct user interaction with Calculator causing invalidation of child controls.
3. Execution of `BWE_ComposeFrame()` synchronously within the hardware Timer Interrupt ISR (`timer_tick_handler`).
