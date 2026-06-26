# 🐭 PS/2 Mouse Fix — Post-Mortem Report

**Date:** 2026-06-26  
**Duration to fix:** ~9 hours  
**Status:** ✅ RESOLVED  
**Severity:** Critical (OS completely unusable without mouse)

---

## Problem Statement

Mouse cursor visible on screen but **instantly teleports to screen corner** on any movement.  
Even with a cheap ₹200 default-DPI mouse, the issue persisted.  
Mouse would literally get "thrown outside" the VirtualBox emulator window within milliseconds of any touch.

---

## Root Causes Found (3 bugs)

### 🔴 Root Cause #1: VirtualBox Pointing Device Misconfiguration (PRIMARY)

**The `.vbox` config had NO `<Pointing>` tag**, meaning VirtualBox used its default: **"USB Tablet"**.

USB Tablet sends **absolute coordinates** (like a touchscreen: X=500, Y=300).  
Our OS expects **relative PS/2 deltas** (dx=+5, dy=-3).

Since our OS has no USB HID drivers, VirtualBox attempted a PS/2 fallback — but the data was **corrupted/mismatched**, causing the cursor to teleport instantly.

**Fix:**
```powershell
& "D:\VM-BOX\VBoxManage.exe" modifyvm "SignaturesATOMS-OS" --mouse ps2
```

**Verification:**
```powershell
& "D:\VM-BOX\VBoxManage.exe" showvminfo "SignaturesATOMS-OS" --machinereadable | Select-String "mouse"
# Output: hidpointing="ps2mouse"
```

> [!CAUTION]
> **EVERY TIME** a new VirtualBox VM is created for SignaturesOS, run `--mouse ps2` FIRST.  
> Without this, mouse will NEVER work correctly. This is not an OS bug — it's a VirtualBox default that doesn't suit bare-metal OS development.

---

### 🟡 Root Cause #2: Signed/Unsigned Type Mismatch in Clamping

**File:** `kernel/input/input.c`

```c
// BUG: global_mouse_x is int32_t (signed), g_kernel_screen_width is uint32_t (unsigned)
// When global_mouse_x is negative, C promotes it to a HUGE unsigned value
// This means the >= comparison NEVER fires, and clamping is SKIPPED!

// BEFORE (broken):
if (global_mouse_x >= g_kernel_screen_width) ...

// AFTER (fixed):
if (global_mouse_x >= (int32_t)g_kernel_screen_width) ...
```

> [!WARNING]
> In C, `signed_var >= unsigned_var` silently promotes the signed value to unsigned.  
> A negative `int32_t` becomes a massive `uint32_t` (~4 billion), so the check always fails.  
> **ALWAYS cast to the signed type when comparing mixed types in clamping logic.**

---

### 🟢 Root Cause #3: Hardcoded Screen Resolution

**File:** `kernel/input/input.c`

Mouse was being clamped to hardcoded `1920x1080`, but VirtualBox window could be any size (1024x768, etc.).

**Fix:** Created global variables `g_kernel_screen_width` / `g_kernel_screen_height` in `kernel.c`, populated from actual VBE mode at boot, and used via `extern` in `input.c`.

---

## Files Modified

| File | Change |
|------|--------|
| `drivers/input/ps2/mouse.c` | Added timeout sync recovery, sensitivity scaling (`/2`) |
| `kernel/input/input.c` | Dynamic resolution clamping, signed/unsigned cast fix |
| `kernel/kernel.c` | Added `g_kernel_screen_width/height` globals from VBE |
| VirtualBox VM Config | `--mouse ps2` via VBoxManage |

---

## Lessons Learned

1. **Check VirtualBox VM config FIRST** — Not all problems are in our OS code. The emulator's default settings can completely break hardware assumptions.

2. **USB Tablet ≠ PS/2 Mouse** — These are fundamentally different protocols (absolute vs relative). A bare-metal OS without USB drivers MUST use PS/2 mouse mode.

3. **C type promotion is silent and deadly** — `int32_t >= uint32_t` does NOT do what you think. Always cast explicitly.

4. **Don't hardcode screen resolution** — Always read from the actual display mode (VBE/VESA) at boot time.

5. **DPI/sensitivity is a red herring** when the underlying protocol is wrong — No amount of `dx/4` scaling fixes corrupted absolute-to-relative data.

---

## Quick Setup Checklist for New VMs

```powershell
# 1. Create VM normally in VirtualBox GUI
# 2. IMMEDIATELY run:
& "path\to\VBoxManage.exe" modifyvm "VM_NAME" --mouse ps2

# 3. Verify:
& "path\to\VBoxManage.exe" showvminfo "VM_NAME" --machinereadable | Select-String "mouse"
# Expected: hidpointing="ps2mouse"

# 4. In VirtualBox GUI: Input > Mouse Integration > UNCHECK
# 5. Boot OS and test mouse
```

---

> **"9 hours ki debugging, 1 line ka VBoxManage command."**  
> — SignaturesOS Dev Log, June 2026
