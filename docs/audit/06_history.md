# AUDIT PART 6: COMMIT HISTORY, VMX-CRITICAL HUNKS & STRING AUDIT
**Document**: `docs/audit/06_history.md`  
**Classification**: READ-ONLY AUDIT - EVIDENCE-BASED  
**Date**: 2026-11-01  
**Scope**: Commit log since baseline, VMX hunks analysis, hardcoded strings, and debunking false claims.

---

## 1. Verified Repository State & Tag Hashes

- **Active Branch**: `feat/freebsd-usb-runtime-v1` [VERIFIED-BY-CODE: `git status`]
- **Verified Git Tags and Hashes**:
  - `baseline-shell-ok` = **`4534cc61a054a2aae2edbe9ff484ed03a0633dfa`** [VERIFIED-BY-CODE: `git rev-parse baseline-shell-ok`]
    - *Clarification on `e42662fa`*: An earlier AI report incorrectly cited `e42662fa`. `git log` confirms `e42662fa` does not exist in the repository; `4534cc61` is the true commit.
  - `milestone-freebsd-keyboard-ok` = **`3bd4f7e987afd699d0ab4d999830cfa81f04fb0e`** [VERIFIED-BY-CODE: `git rev-parse milestone-freebsd-keyboard-ok`]
    - *Clarification on `6082e235`*: Commit `6082e235` was a documentation update, while the milestone tag itself points to `3bd4f7e9`.
  - `HEAD` = **`46244630`** (Audit documentation commit).

---

## 2. Commit History Since `baseline-shell-ok` (`4534cc61`)

Total: 17 commits.

| Commit Hash | Author | One-Line Summary | Touched Files | Touched VMX Core? |
|---|---|---|---|---|
| **`36e4a9a1`** | Saumya Chaudhari | stage-a: add live telemetry counters for HLT exit rate, pause spin TSC cycles, and vCPU run time | `hypervisor.c`, `hypervisor.h` | **YES** |
| **`260057bd`** | Saumya Chaudhari | fix(hypervisor): eliminate host busy-wait HLT pause loop and implement cooperative vcpu halted scheduling | `hypervisor.c` | **YES** |
| **`1d51f869`** | Saumya Chaudhari | feat(hypervisor): bound vcpu slice by wall-clock duration to prevent starvation | `hypervisor.c` | **YES** |
| **`a16540b1`** | Saumya Chaudhari | feat(input): implement unified host-guest input focus router state machine | `virtual_platform.c`, `usb_hid.c`, `hypervisor_dashboard.c` | NO |
| **`f8cf593a`** | Saumya Chaudhari | feat(hypervisor): implement HID-to-ASCII UART serial console routing and IRQ4 injection | `virtual_platform.c`, `hypervisor.c`, `usb_hid.c` | **YES** |
| **`24b545c2`** | Saumya Chaudhari | feat(input): implement virtual i8042 PS/2 controller, IRQ1 injection, scancode sets 1/2 | `virtual_platform.c`, `hypervisor.c`, `usb_hid.c` | **YES** |
| **`fb8f4b03`** | Saumya Chaudhari | fix(hypervisor): eliminate fallback interrupt injection, track 8259 PIC ICW2 base | `hypervisor.c`, `virtual_platform.c`, `freebsd_loader.c` | **YES** |
| **`6de1e186`** | Saumya Chaudhari | feat(debug): implement STAGE D0 dual UDP telemetry streams, dmesg reader | `hypervisor.c`, `hypervisor_dashboard.c` | **YES** |
| **`6a9c37ce`** | Saumya Chaudhari | chore(build): update build_identity timestamp for STAGE D0 | `build_identity.h` | NO |
| **`360a4f3d`** | Saumya Chaudhari | feat(acpi): integrate KBD (PNP0303) and COM1 (PNP0501) into DSDT, fix USB HID LED sync | `virtual_platform.c`, `usb_hid.c` | NO |
| **`6082e235`** | Saumya Chaudhari | docs(hypervisor): document FreeBSD 14.1 interactive keyboard & ACPI DSDT certification | `README.md`, docs | NO |
| **`e3e3556a`** | Saumya Chaudhari | feat(telemetry): instrument end-to-end keystroke latency profiling across 6 stages | `hypervisor.c`, `virtual_platform.c`, `usb_hid.c` | **YES** |
| **`aa9194b5`** | Saumya Chaudhari | feat(hypervisor): resolve fast-typing lag via i8042 queue pipeline fixes, 30fps display flush | `hypervisor.c`, `virtual_platform.c`, `hypervisor_dashboard.c` | **YES** |
| **`e7ae099a`** | Saumya Chaudhari | docs(hypervisor): document Type-1 Hypervisor architecture tree | docs | NO |
| **`b9625105`** | Saumya Chaudhari | feat(hypervisor): support non-resident NTFS attribute list, rate-limit reset log, fix modifiers | `runtime_storage.c`, `hypervisor.c`, `virtual_platform.c`, `usb_hid.c` | **YES** |
| **`e98ace6f`** | Saumya Chaudhari | fix(storage): enable has_data on alist_file and ensure block device binding for NTFS | `runtime_storage.c`, `hypervisor.c` | NO |
| **`46244630`** | Saumya Chaudhari | docs(audit): comprehensive read-only hypervisor, storage, and input audit | `docs/audit/*` | NO |

---

## 3. Forensic Analysis of VMX-Critical Code Hunks

| Commit | File & Lines | What Was Modified | Architectural Rationale | Status Label | Evidence |
|---|---|---|---|---|---|
| **`260057bd`** | `hypervisor.c:1742-1810` | Replaced 10,000-cycle host pause spin on `VMX_EXIT_REASON_HLT` with `vcpu->halted = true` and cooperative break. | Host CPU was burning 100% cycles spinning on guest idle, starving USB and network drivers. | `UNVERIFIED` (anecdotal claims of "CPU temps dropped" lack hardware thermals logs) | Code inspection confirms cooperative break. |
| **`1d51f869`** | `hypervisor.c:4030-4050` | Added RDTSC wall-clock check (`elapsed >= 5250000` cycles = 1.5ms) inside vCPU exit loop. | Prevented a tight guest loop (e.g. `rep nop` without exits) from starving host input polling. | `VERIFIED-BY-CODE` | Code bounds step to ~1.5ms. |
| **`24b545c2`** | `hypervisor.c:770-815` | Implemented `atoms_hypervisor_try_inject_kbd_irq()` to inject IOAPIC RTE[1] vector on VM entry. | Required to deliver PS/2 scancodes into FreeBSD `atkbd` driver. | `VERIFIED-ON-HARDWARE` | User typed commands interactively in FreeBSD shell. |
| **`fb8f4b03`** | `hypervisor.c:833-875` | Removed unconditional fallback injection of PIC IRQ 1 (vector `0x21`). | When vector `0x21` was injected prior to FreeBSD IDT readiness, it caused FreeBSD trap 30 / double fault. (Earlier claim of "vector 0x31" and "vmstat 32,000/s" was an unverified text error). | `VERIFIED-BY-CODE` | Git diff of `fb8f4b03` shows `vector = 0x21;` replaced with `can_inject = false;`. |
| **`b9625105`** | `hypervisor.c:1925-1940` | Added intercept for Port 0xCF9. Sets `disposition = VMEXIT_GUEST_RESET`. | Intercepts guest hardware reboot without unhandled exit crash. | `VERIFIED-BY-CODE` | Handled at line 1926. |

---

## 4. Debunking False Claims & Hardcoded Strings

1. **Motherboard Hallucination**:
   - Earlier AI reports claimed tests ran on "Intel H81 Haswell motherboard".
   - **Fact**: Live hardware MAC `A0:AD:9F:C5:81:27`, CPUID `0x000B06F2`, and UEFI firmware confirm physical test bed is **ASUS PRIME B760M-K (Intel Core i3-14100F Raptor Lake)**.
2. **"Zero VMX Changes" Claim**:
   - Earlier report claimed "ZERO VMX changes were made".
   - **Fact**: Commits `36e4a9a1`, `260057bd`, `1d51f869`, `f8cf593a`, `24b545c2`, `fb8f4b03`, `6de1e186`, `e3e3556a`, `aa9194b5`, and `b9625105` (10 commits) all directly modified `hypervisor.c`.
3. **Hardcoded Status Strings**:
   - `hypervisor.c:3672`: `com1_puts("Pre-Flight Consistency Gate: 100% PASS\n");` is hardcoded.
   - `virtio_blk.c:134`: `com1_puts("[VIRTIO-BLK] Boundary Verification ... : 100% PASS\n");` is hardcoded.
   - `hypervisor_dashboard.c:2860`: Returns `"PASS"` whenever `!g_hv_dashboard.halted`.
4. **Exit Reason 0x30 Mislabeling**:
   - Prior chat summary mislabeled `ExitReason=0x00000030` as an "I/O exit".
   - **Fact**: Per Intel SDM, hex `0x30` = Dec 48 = `EXIT_REASON_EPT_VIOLATION` (MMIO exit). Decimal 30 = Hex `0x1E` = `EXIT_REASON_IO_INSTRUCTION`.

---

## 5. Top 10 Claims Least Sure About (Part 6)

1. Whether commit `b9625105` introduced any subtle timing drift in the 1.5ms vCPU quantum check.
2. Whether the RDTSC latency probes in `e3e3556a` add measurable serialization overhead on Raptor Lake.
3. The exact date/time when tag `baseline-shell-ok` was originally created on git.
4. Whether commit `aa9194b5` (30fps display flush) drops frames when the guest writes high-frequency console output.
5. If `vmm_rdmsr()` calls in `vmx_adjust_controls()` can cause VM-exit storms if invoked repeatedly in tight loops.
6. The precise RAM footprint difference between the embedded 14.1 kernel and the 14.5 USB kernel.
7. Whether any BIOS revisions of the ASUS B760M-K disable unrestricted guest mode by default.
8. If the IOAPIC RTE[1] vector allocation varies when FreeBSD boots with ACPI disabled (`hint.acpi.0.disabled=1`).
9. Behavior of `atoms_hypervisor_reinit_vm()` if invoked while a VirtIO-BLK read request is in-flight.
10. The exact reason why earlier AI reports hallucinated `e42662fa` as the baseline hash.
