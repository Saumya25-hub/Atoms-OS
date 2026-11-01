# ATOMS OS — PHASE 1 ROLLBACK RECORD

**Subsystem:** USB HID Class Driver & xHCI Transfer Engine  
**Checkpoint Commit:** `da00b6ed1fd4499de304e02a11d807863d847516`  
**Branch:** `feat/vmx-mouse-diag-v1`  
**Date:** 2026-11-02  
**Status:** BACKUP ACTIVE & ARMED  

---

## 1. Preserved Backup Checkpoints

Two physical backup copies were generated in the repository before any code modifications were applied:

1. `kernel/drivers/usb/class/usb_hid.c.phase1.bak`
   - SHA-256 Checksum: Verified intact prior to patch.
   - Preserves monolithic `usb_hid_bind` and legacy report dispatch.
2. `kernel/drivers/usb/host/xhci/xhci_transfer.c.phase1.bak`
   - SHA-256 Checksum: Verified intact prior to patch.
   - Preserves legacy slot-based transfer event handler.

---

## 2. Emergency Rollback Instructions

If unexpected behavior or regression is observed on bare-metal hardware during Phase 1 deployment:

### Method A: Instant Physical File Restoration (Recommended)
Execute in PowerShell:
```powershell
Copy-Item -Force kernel\drivers\usb\class\usb_hid.c.phase1.bak kernel\drivers\usb\class\usb_hid.c
Copy-Item -Force kernel\drivers\usb\host\xhci\xhci_transfer.c.phase1.bak kernel\drivers\usb\host\xhci\xhci_transfer.c
powershell -ExecutionPolicy Bypass -File .\build.ps1
```

### Method B: Git Branch Rollback
Execute with Git:
```powershell
$env:GIT_CONFIG_GLOBAL = "NUL"
git checkout da00b6ed1fd4499de304e02a11d807863d847516 -- kernel/drivers/usb/class/usb_hid.c kernel/drivers/usb/host/xhci/xhci_transfer.c
powershell -ExecutionPolicy Bypass -File .\build.ps1
```

---

## 3. Post-Rollback Validation Checklist

After executing rollback, verify that:
1. `git diff kernel/drivers/usb/class/usb_hid.c` returns 0 changes against checkpoint commit `da00b6ed`.
2. `git diff kernel/drivers/usb/host/xhci/xhci_transfer.c` returns 0 changes against checkpoint commit `da00b6ed`.
3. `./build.ps1` runs cleanly and generates `build/OS.img` and `build/atoms_uefi_test.img`.
4. QEMU boots without unresolved symbols.
