# ATOMS OS — STORAGE DRIVER DEVELOPMENT WORKFLOW

**Document ID**: `docs/STORAGE_DRIVER_WORKFLOW.md`  
**Purpose**: Authoritative SOP & Engineering Protocol for Future Developers and AI Agents  
**Target Subsystem**: ATOMS OS Storage Stack & Hardware Controller Drivers  
**Governing Rule**: RULE 0 (Phase Isolation: Investigate ➔ Plan ➔ Patch ➔ Certify)

---

## 1. Quick-Start Guide for Future Developers & AI Agents

If you are tasked with continuing ATOMS storage development, **YOU MUST FOLLOW THIS EXACT SEQUENCE BEFORE TOUCHING CODE**:

1. **Read `docs/STORAGE_ARCHITECTURE.md` first**: Understand the master storage tree, subsystem boundaries, and dynamic discovery principles.
2. **Read the specific driver documentation package**:
   - For AHCI: `docs/drivers/storage/ahci/`
   - For NVMe: `docs/drivers/storage/nvme/`
   - For USB Mass Storage: `docs/drivers/storage/usb_mass_storage/`
3. **Consult the Driver Status Registry**: Confirm which driver is currently active, which are certified/frozen, and which are scheduled next.
4. **Obey the Inviolable Storage Laws**:
   - **NEVER** modify a certified/frozen driver without a formal, approved forensic case.
   - **NEVER** modify NTFS, FAT32, or VFS core to work around a controller driver bug.
   - **NEVER** mix hardware controller code (registers, MMIO, DMA) with filesystem code.
   - **NEVER** hardcode physical disk identities (capacities, serials, sectors, or manufacturer names).
   - **ALWAYS** discover actual hardware dynamically from PCI and IDENTIFY data.
   - **ALWAYS** test **READ-ONLY** first. Absolutely zero disk writes, formatting, or partition modifications during bring-up.
   - **ALWAYS** produce verifiable telemetry evidence (serial logs, UDP framebuffer screenshots) before claiming PASS.
   - **NEVER** declare a physical storage PASS merely because an in-memory fallback (DummyFS) mounted.
5. **Freeze Completed Drivers**: Once a driver passes real hardware certification, freeze it and move to the next driver.

---

## 2. The 19-Step Storage Driver Implementation Lifecycle

Every storage controller driver in ATOMS OS must execute each step in strict sequence:

```text
 1. Source & Hardware Audit        ──> Verify PCI devices, vendor/device IDs, BARs, and registers
 2. Linux Reference Study          ──> Study Linux libata/nvme implementation and DMA mechanisms
 3. Windows NT Reference Study     ──> Study Windows Storport / Miniport architecture separation
 4. ATOMS Architecture Mapping     ──> Map hardware primitives to clean ATOMS BlockDevice interface
 5. Dependency Audit               ──> Verify PMM, VMM, and PCI APIs required by the driver
 6. Driver Design Document         ──> Create permanent driver design and register specification
 7. Minimal Patch Plan             ──> Author PATCH_PLAN.md listing ONLY relevant files
 8. Git Pre-Implementation Check   ──> Take clean git status and commit checkpoint
 9. Driver Implementation          ──> Author clean, isolated driver in kernel/drivers/storage/
10. Compilation & Clean Build      ──> Compile with zero warnings/errors via build.ps1
11. QEMU Pre-Flight Validation     ──> Validate driver against QEMU pure UEFI environment
12. Physical Hardware Bring-Up     ──> Deploy to bare-metal target (ASUS B750M-K)
13. Read-Only Stress Probe         ──> Non-destructive multi-sector reads, header validations
14. Error Resilience Verification  ──> Verify bounded timeouts, missing drive handling, link loss
15. Forensic Telemetry Review      ──> Capture UDP screen frame & serial logs to verify PASS
16. Formal Milestone Certification ──> Author CERTIFICATION_REPORT.md with binary PASS/FAIL
17. Git Certification Commit       ──> Commit certified milestone with descriptive message
18. FREEZE DRIVER                  ──> Mark driver FROZEN in Driver Status Registry
19. Advance to Next Driver         ──> Proceed to next controller family on roadmap
```

---

## 3. Strict Phase Isolation (RULE 0)

| Phase | Team / Agent Role | Permitted Actions | Prohibited Actions | Mandatory Deliverable |
| :--- | :--- | :--- | :--- | :--- |
| **Phase 1** | Forensic Team | Read files, read logs, inspect telemetry, audit registers. | **NO CODE MODIFICATIONS.** No editing source files. | `FORENSIC_REPORT.md` |
| **Phase 2** | Architecture Team | Review forensic report, design data structures, map dependencies. | **NO CODE MODIFICATIONS.** No speculative coding. | `PATCH_PLAN.md` |
| **Phase 3** | Implementation Team | Modify **ONLY** files explicitly approved in `PATCH_PLAN.md`. | No touching unrelated subsystems. No refactoring. | `PATCH_REPORT.md` |
| **Phase 4** | Certification Team | Build, run QEMU test, flash bare metal, analyze telemetry. | No adding new features. | `CERTIFICATION_REPORT.md` |

---

## 4. Git Checkpoint & Commit Policy

Before writing a single line of driver code:
```bash
git status
# Confirm working tree is clean and on the designated branch
```

Commit message conventions for the storage program:
- Pre-implementation checkpoint: `chore(storage): pre-implementation checkpoint for <driver>`
- Driver feature addition: `feat(storage): implement native <driver> driver (read-only)`
- Milestone certification: `cert(storage): <driver> real hardware certification PASS`

---

## 5. Protected Subsystems List

If any proposed storage change appears to require modifications to the following files, **STOP IMMEDIATELY AND PERFORM AN ARCHITECTURAL AUDIT FIRST**:

1. **UEFI Bootloader**: `boot/`
2. **Kernel Entry & Early CPU Setup**: `kernel/kernel.c`, `arch/x86_64/`
3. **Core Memory Management**: `kernel/core/memory/pmm/`, `kernel/core/memory/vmm/`
4. **Display & Diagnostic Engine**: `kernel/debug/abde/`, `kernel/display/dgl/`
5. **Certified USB / xHCI HID Engine**: `kernel/drivers/usb/` (Mouse/Keyboard certification must not be compromised)
6. **Syscall Security Boundary**: `kernel/core/syscall/`
7. **Filesystem Internals**: `kernel/vfs/vfs_legacy/fs/ntfs/`, `kernel/vfs/vfs_legacy/fs/fat32/`
