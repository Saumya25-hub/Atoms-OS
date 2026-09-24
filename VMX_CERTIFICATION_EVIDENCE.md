# ATOMS OS — Intel VT-x Hardware Virtualization & Certification Evidence

**Kernel Version:** `v2.7.0-vmx-stable`  
**Silicon Milestone:** Intel VT-x (VMX) Type-1 Hardware Hypervisor & EPT Paging Certified on Physical LGA1700 Silicon (Intel Core i3-14100F, ASUS PRIME B760M-K)  
**Primary Documentation:** [docs/hypervisor/ATOMS_VMX_HARDWARE_CERTIFICATION.md](file:///d:/Signatures_OS/docs/hypervisor/ATOMS_VMX_HARDWARE_CERTIFICATION.md)

---

### Quick Evidence Index

1. **Hardware Specification & Micro-Hypervisor Architecture**:
   - See [Section 2 & 3 in ATOMS_VMX_HARDWARE_CERTIFICATION.md](file:///d:/Signatures_OS/docs/hypervisor/ATOMS_VMX_HARDWARE_CERTIFICATION.md#2-physical-hardware-test-rig-specification).
2. **Real Bare-Metal Telemetry Logs**:
   - `VMLAUNCH SUCCESS! (CF=0, ZF=0)`
   - Exit Reason `0x0A` (CPUID) at RIP `0xFFFFFFFF80FD1D76`
   - High-volume exit handling (`0x7A120` / 500,000 bare-metal exits, 28.4M QEMU exits)
   - Port `0xCF9` guest reset autopsy.
   - See [Section 4 in ATOMS_VMX_HARDWARE_CERTIFICATION.md](file:///d:/Signatures_OS/docs/hypervisor/ATOMS_VMX_HARDWARE_CERTIFICATION.md#4-verbatim-hardware-telemetry-logs).
3. **Physical Screen Captures (Visual Proof)**:
   - Framebuffer captures saved directly from physical hardware runs are located in [artifacts/screenshots/](file:///d:/Signatures_OS/artifacts/screenshots/):
     - [forensic_screen_20260922_194127_s99.png](file:///d:/Signatures_OS/artifacts/screenshots/forensic_screen_20260922_194127_s99.png) (Stage 99 Registers & Paging)
     - [forensic_screen_20260922_202244_s99.png](file:///d:/Signatures_OS/artifacts/screenshots/forensic_screen_20260922_202244_s99.png) (VirtIO Subsystem Probing)
     - [forensic_screen_20260922_210830_s99.png](file:///d:/Signatures_OS/artifacts/screenshots/forensic_screen_20260922_210830_s99.png) (Hypervisor 6-Card Dashboard)
     - [forensic_screen_20260922_222143_s99.png](file:///d:/Signatures_OS/artifacts/screenshots/forensic_screen_20260922_222143_s99.png) (xHCI Physical Keyboard Telemetry)
     - [forensic_screen_20260922_222224_s99.png](file:///d:/Signatures_OS/artifacts/screenshots/forensic_screen_20260922_222224_s99.png) (VirtIO Network Debug Screen)
     - [forensic_screen_20260922_222318_s99.png](file:///d:/Signatures_OS/artifacts/screenshots/forensic_screen_20260922_222318_s99.png) (VirtIO Graphics Debug Screen)
4. **Subsystem Forensics & Audit Protocols**:
   - [FORENSIC_REPORT.md](file:///d:/Signatures_OS/FORENSIC_REPORT.md) — Physical Keyboard Input Deep Audit & xHCI Event Ring Dequeue Analysis
   - [PATCH_PLAN.md](file:///d:/Signatures_OS/PATCH_PLAN.md) — Isolated Architectural Modification Plan
   - [PATCH_REPORT.md](file:///d:/Signatures_OS/PATCH_REPORT.md) — Exact Code Changes & Telemetry Bindings
   - [CERTIFICATION_REPORT.md](file:///d:/Signatures_OS/CERTIFICATION_REPORT.md) — Full Milestone Pass/Fail Breakdown
