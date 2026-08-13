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

---

## Target Physical Hardware Profile

- **Motherboard**: H81 Motherboard (Haswell LGA1150 Chipset)
- **BIOS Firmware**: 2022 Updated BIOS (Native UEFI Mode)
- **CPU**: Intel Core i3 4th Gen (Haswell x86_64)
- **RAM**: 8 GB RAM

---

# ATOMS OS Engineering Protocol V1

## RULE 0: Mandatory Phase Isolation
Patch karne ki permission tab tak nahi jab tak Forensic Investigation aur Architecture Plan approved na ho.
Investigate ➔ Plan ➔ Patch ➔ Certify.

---

## TASK 1 — FORENSIC TEAM
- **Allowed**: Read files, read logs, read docs, read telemetry, read architecture.
- **Not Allowed**: Edit files, create patches, refactor.
- **Output**: `FORENSIC_REPORT.md` (Root cause, Evidence, Files involved, Risk analysis, Suspected fix - NO CODE).

---

## TASK 2 — ARCHITECT TEAM
- **Input**: `FORENSIC_REPORT.md`
- **Allowed**: Read source, read report.
- **Not Allowed**: Edit source.
- **Output**: `PATCH_PLAN.md` (What to modify, Why, Expected result, Risk, Rollback plan - NO CODE).

---

## TASK 3 — PATCH TEAM
- **Input**: `FORENSIC_REPORT.md`, `PATCH_PLAN.md`
- **Allowed**: Modify ONLY files listed in `PATCH_PLAN.md`.
- **Not Allowed**: Touch unrelated files or refactor random code.
- **Output**: `PATCH_REPORT.md` (Files changed, Functions changed, Lines changed).

---

## TASK 4 — CERTIFICATION TEAM
- **Input**: Patched build.
- **Allowed**: Automated & Manual Testing only.
- **Output**: `CERTIFICATION_REPORT.md` (PASS / FAIL, Regression list, New bugs found).

---

## HARD RULES
❌ Refactor random files
❌ Touch unrelated subsystems
❌ Rewrite architecture
❌ "While I'm here I'll improve this"
❌ Create new engines
❌ Rename APIs
❌ Touch `kernel.c` unless explicitly approved in `PATCH_PLAN.md`


