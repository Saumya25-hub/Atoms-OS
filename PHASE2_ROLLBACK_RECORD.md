# ATOMS OS — PHASE 2 ROLLBACK RECORD

**Phase:** Phase 2 — HRE (Universal HID 1.11 Bytecode Parser Engine)  
**Date:** 2026-11-02  
**Baseline Git Checkpoint:** `v2.0-mouse-stable` (Commit `4b6bb5c52c28ae2361b2b80017178cfba488d591`)  
**Target Git Branch:** `feat/vmx-mouse-diag-v1`  

---

## 1. Rollback Procedure

If any regression occurs on bare-metal hardware during Phase 2 deployment, the operator or engineer can instantly revert to the certified Phase 1 baseline using any of the following procedures:

### Option A: Complete Git Reset to Certified Baseline
```powershell
[Environment]::SetEnvironmentVariable('GIT_CONFIG_GLOBAL', 'NUL', 'Process')
git checkout feat/vmx-mouse-diag-v1
git reset --hard v2.0-mouse-stable
powershell -ExecutionPolicy Bypass -File .\build.ps1
```

### Option B: Isolated File Reversion
If only the USB stack needs to be reverted:
```powershell
[Environment]::SetEnvironmentVariable('GIT_CONFIG_GLOBAL', 'NUL', 'Process')
git checkout v2.0-mouse-stable -- kernel/drivers/usb/class/usb_hid.c build.ps1
Remove-Item kernel/drivers/usb/class/hid_parser.c, kernel/drivers/usb/class/hid_parser.h -Force
powershell -ExecutionPolicy Bypass -File .\build.ps1
```

---

## 2. Integrity Verification

The Phase 1 certified binaries and tag (`v2.0-mouse-stable`) are fully pushed and preserved on GitHub:
- Remote: `https://github.com/Saumya25-hub/Atoms-OS.git`
- Branch: `feat/vmx-mouse-diag-v1`
- Tag: `v2.0-mouse-stable`
