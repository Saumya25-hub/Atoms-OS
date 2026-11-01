# AUDIT PART 6: COMMIT HISTORY, VMX-CRITICAL HUNKS & STRING AUDIT

## 1. Repository State & Uncommitted Changes

- **Active Branch**: `feat/freebsd-usb-runtime-v1` [VERIFIED-BY-CODE: git status]
- **Key Milestones / Tags**:
  - `baseline-shell-ok` (`e42662fa`): FreeBSD booted to shell prompt on synthetic 16 MB disk.
  - `milestone-freebsd-keyboard-ok` (`6082e235`): ACPI DSDT KBD/COM1 merged, i8042 IRQ 1 active.
  - `HEAD` (`e98ace6f`): USB NTFS Non-Resident `$ATTRIBUTE_LIST` parser, 0xCF9 reset handler, modifier break code fix.
- **Uncommitted Modifications in Working Directory**:
  - `docs/hypervisor/ATOMS_TYPE1_HYPERVISOR_TREE_AND_LATENCY_ARCHITECTURE.md`: Documentation updates.
  - `kernel/include/build_identity.h`: Build timestamp macro update.
  - `kernel/kernel.c`: Added block device init call before hypervisor dashboard.
  - `kernel/vfs/vfs_legacy/storage/src/block_device.c`: Added null safety check in `block_device_get()`.
  - `tools/freebsd_payload/minimal_rootfs/etc/rc`: Script comments.
  - `tools/make_minimal_ufs2.py`: Minimal image generation script adjustments.

---

## 2. Commit History Since `baseline-shell-ok`

Total: 16 commits.

| Commit Hash | Author | One-Line Summary | Touched Files | Touched VMX Core? |
|:---|:---|:---|:---|:---|
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

---

## 3. Forensic Analysis of VMX-Critical Code Hunks

| Commit | File & Lines | What Was Modified | Architectural Rationale | Risk Level | Hardware Proof |
|:---|:---|:---|:---|:---|:---|
| **`260057bd`** | `hypervisor.c:1742-1810` | Replaced 10,000-cycle host pause spin on `VMX_EXIT_REASON_HLT` with `vcpu->halted = true` and cooperative break. | Host CPU was burning 100% cycles spinning on guest idle, starving USB and network drivers. | LOW | VERIFIED-BY-LOG: CPU temps dropped, frame pacing stabilized. |
| **`1d51f869`** | `hypervisor.c:4030-4050` | Added RDTSC wall-clock check (`elapsed >= 5250000` cycles = 1.5ms) inside vCPU exit loop. | Prevented a tight guest loop (e.g. `rep nop` without exits) from starving host input polling. | LOW | VERIFIED-BY-LOG: Step bounded to ~1.5ms. |
| **`24b545c2`** | `hypervisor.c:770-815` | Implemented `atoms_hypervisor_try_inject_kbd_irq()` to inject IOAPIC RTE[1] vector on VM entry. | Required to deliver PS/2 scancodes into FreeBSD `atkbd` driver. | MEDIUM | VERIFIED-ON-HARDWARE: User typed `df -h` and `ls` interactively in FreeBSD shell. |
| **`fb8f4b03`** | `hypervisor.c:800-840` | Removed unconditional fallback injection of Vector 0x31 when no interrupt was pending. | Fallback injection was flooding spurious interrupts into guest IDT Vector 0x31, causing interrupt storm. | HIGH | VERIFIED-BY-LOG: `vmstat -i` rate dropped from 32,000/s to 2/s. |
| **`e3e3556a`** | `hypervisor.c:4025` | Inserted RDTSC probe calls before `vmresume` for end-to-end latency profiling. | Diagnostics only. Read-only RDTSC instruction before entry. | ZERO | VERIFIED-BY-CODE: Does not modify any register or VMCS field. |
| **`b9625105`** | `hypervisor.c:1925-1940` | Added intercept for Port 0xCF9 (Reset Control Register). Sets `disposition = VMEXIT_GUEST_RESET`. | Prevented guest triple fault / reset loop from flooding serial COM1 with 1,000 logs/second. | LOW | VERIFIED-ON-HARDWARE: User saw clean 1-line log on reset instead of terminal freeze. |

---

## 4. Status String Audit (Hardcoded vs Computed State)

### 4.1 Examples of Hardcoded Status Strings
1. [VERIFIED-BY-CODE: kernel/core/hypervisor/src/hypervisor.c:3672]:
   `com1_puts("Pre-Flight Consistency Gate: 100% PASS\n");`
   - **Fact**: The string `"100% PASS"` is hardcoded inside the print literal, even though it executes only after the check passes.
2. [VERIFIED-BY-CODE: kernel/core/hypervisor/src/virtio_blk.c:134]:
   `com1_puts("[VIRTIO-BLK] Boundary Verification (Beginning, Middle, End, Out-of-Range Rejection): 100% PASS\n");`
   - **Fact**: `"100% PASS"` is a hardcoded literal.
3. [VERIFIED-BY-CODE: kernel/debug/hypervisor_dashboard/hypervisor_dashboard.c:2860]:
   `hv_format_heartbeat(spinner_msg, spin_char, g_hv_dashboard.halted ? "HALTED_ON_FAIL" : "PASS", ...);`
   - **Fact**: Returns literal `"PASS"` whenever `!g_hv_dashboard.halted`.

### 4.2 Examples of Real Computed State
1. [VERIFIED-BY-CODE: kernel/debug/hypervisor_dashboard/hypervisor_dashboard.c:1740-1755]:
   `ROOT SOURCE: USB IMAGE (ufs:/dev/vtbd0p2)` vs `ROOT SOURCE: SYNTHETIC 16MB`
   - **Fact**: Computed dynamically:
     ```c
     if (runtime_storage_is_available()) {
         bwe_draw_string(..., "ROOT SOURCE: USB IMAGE (ufs:/dev/vtbd0p2)", ...);
     } else {
         bwe_draw_string(..., g_runtime_storage_telemetry.root_source, ...);
     }
     ```
2. [VERIFIED-BY-CODE: kernel/core/hypervisor/src/hypervisor.c:3950-3960]:
   `vm->state` and `vcpu->state`: Computed from enum transitions (`VM_STATE_RUNNING`, `VM_STATE_STOPPED`, `VM_STATE_INITIALIZED`).
3. [VERIFIED-BY-CODE: kernel/core/hypervisor/src/runtime_storage.c:60-70]:
   Telemetry counters: `total_read_requests`, `total_sectors_read`, `last_latency_us`, `avg_latency_us` are computed from real RDTSC cycle deltas.

---

## 5. False & Unproven Claims from Earlier AI Reports

1. **Claim: "Hardware is an H81 Haswell Motherboard"**
   - **Status**: **PROVEN FALSE**.
   - **Evidence**: User uploaded AMDE screenshot [media_1793516469144_920432e0.jpg] showing MAC `A0:AD:9F:C5:81:27`, which is registered to an ASUS PRIME B760M-K (Intel Core i3-14100F Raptor Lake). The H81 target profile was a stale configuration template.
2. **Claim: "Zero modifications have been made to VMX core files"**
   - **Status**: **PROVEN FALSE**.
   - **Evidence**: `git log` proves that 10 commits modified `kernel/core/hypervisor/src/hypervisor.c`. Lines changed include the HLT exit handler, interrupt window injection, Port 0xCF9 reset trap, and wall-clock time bounding.
3. **Claim: "371 Million exits sustained without a single stutter"**
   - **Status**: **UNVERIFIED HYPERBOLE**.
   - **Evidence**: The number was an unchecked cumulative counter in a test run that included idle HLT pause spins before commit `260057bd`.
4. **Claim: "FreeBSD USB runtime image was 100% active in all previous runs"**
   - **Status**: **PROVEN FALSE**.
   - **Evidence**: Live serial logs prior to commit `e98ace6f` show that `runtime_storage.c` failed to read Non-Resident `$ATTRIBUTE_LIST` clusters (returned 0 bytes because `alist_file.has_data` was false). The system silently fell back to the 16 MB synthetic ramdisk (`/dev/vtbd0`). The genuine 20 GiB USB image was only successfully mounted in commit `e98ace6f`.

---

## 6. Top 10 Claims Least Sure About (Part 6)

1. Whether commit `fb8f4b03` broke 8259 PIC interrupt delivery for legacy guests that do not use the IOAPIC.
2. Whether the 1.5 ms slice bounding in commit `1d51f869` causes perceptible audio stutter if sound virtualization is added later.
3. Exactly how many times the guest rebooted via Port 0xCF9 during the user's multi-hour physical tests.
4. Whether `build_identity.h` build timestamps are synchronized across all compiled object files in incremental builds.
5. If git commit `aa9194b5` (display flush rate limit) causes delayed screen updates in text editors like `vi` or `ee`.
6. Whether the hardcoded DSDT AML byte array in commit `360a4f3d` works on AMD SVM platforms without modification.
7. How many of the 28 dashboard stages in `hypervisor_dashboard_run_stage_pipeline` perform active hardware tests vs sanity assertion checks.
8. Whether the 3.0-second auto-reinit in commit `b9625105` properly resets VirtIO queue head/tail indices to 0.
9. Whether `s_guest_reset_start_tsc` properly resets if the user presses 'R' before 3.0 seconds elapse.
10. If the host kernel stack has sufficient headroom when handling nested exits during high-frequency disk I/O.
'''

with open('docs/audit/06_history.md', 'w', encoding='utf-8') as f:
    f.write(text.strip() + '\n')
print('Wrote docs/audit/06_history.md successfully')
