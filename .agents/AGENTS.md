# ATOMS OS — Development & Hardware Bring-Up Rules

## Mandatory Pre-Flash Verification Rule

Before requesting a USB flash cycle or producing a new USB image for physical H81 hardware testing:

1. **Build**: Compile kernel and bootloader cleanly with zero errors.
2. **QEMU Pre-Flight**: Boot in QEMU in pure UEFI mode.
3. **ABDE Rendering Verification**: Verify ABDE diagnostic table renders cleanly on screen.
4. **Step Verification**: Verify expected diagnostic step appears.
5. **Heartbeat Spinner Verification**: Verify heartbeat spinner is actively rotating (`| / - \`).
6. **No Regression**: Verify no regressions from previous certified stages.

Only after QEMU pre-flight validation succeeds may a USB test image be produced for physical bare-metal hardware.

---

## Physical Hardware Milestone Certification Protocol

Physical hardware testing on real H81 motherboard is reserved strictly for formal milestone certifications:
- **CPU Certification**
- **GDT Certification**
- **SMP Certification**
- **IDT Certification**
- **PIC Certification**
- **PMM Certification**
- **VMM Certification**
- **Heap Certification**

Every physical hardware test must answer a specific forensic question and yield an unambiguous binary **PASS / FAIL** verdict.
