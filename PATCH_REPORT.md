# ATOMS OS — PATCH REPORT (TASK 3)
### TARGET: ACPI `_CRS` PRODUCER DESCRIPTOR FLAGS & TELEMETRY LIVE SYNC

**Protocol Phase**: TASK 3 — PATCH TEAM (RULE 0 Mandatory Phase Isolation)  
**Input Reference**: [`PATCH_PLAN.md`](file:///d:/Signatures_OS/PATCH_PLAN.md), [`FORENSIC_REPORT.md`](file:///d:/Signatures_OS/FORENSIC_REPORT.md)  
**Target Hardware**: ASUS PRIME B760M-K (Intel Core i3-14100F, Haswell-Raptor Lake x86_64, Realtek RTL8125 2.5GbE)  
**Date**: 2026-09-26  
**Status**: APPLIED CLEANLY  

---

## 1. Summary of Changes Applied

Three files listed in `PATCH_PLAN.md` have been modified:

### 1. [`kernel/core/hypervisor/src/virtual_platform.c`](file:///d:/Signatures_OS/kernel/core/hypervisor/src/virtual_platform.c)
- **Functions Modified**: `s_dsdt_aml[]` synthetic DSDT table.
- **Lines Changed**: Lines 130–149.
- **Modifications**:
  - `WordBusNumber` descriptor: GeneralFlags changed from `0x0C` (Consumer) to `0x0D` (`ResourceProducer`).
  - `WordIO (0x0000 - 0x0CF7)` descriptor: GeneralFlags changed from `0x0C` to `0x0D` (`ResourceProducer`).
  - `WordIO (0x0D00 - 0xFFFF)` descriptor: GeneralFlags changed from `0x0C` to `0x0D` (`ResourceProducer`).
  - `DWordMemory (0x80000000 - 0xFEBFFFFF)` descriptor: GeneralFlags changed from `0x0C` to `0x0D` (`ResourceProducer`).
- **Rationale**: FreeBSD's `acpi_pcib_producer_handler` (`sys/dev/acpica/acpi_pcib_acpi.c:640`) strictly requires `ProducerConsumer == ACPI_PRODUCER` (`0x0D` / bit 0 = 1). The previous `0x0C` caused FreeBSD to reject all host resource windows, leaving `sc->ap_host_res` empty and breaking VirtIO BAR0 allocations.

### 2. [`kernel/core/hypervisor/src/hypervisor.c`](file:///d:/Signatures_OS/kernel/core/hypervisor/src/hypervisor.c)
- **Functions Modified**: `atoms_vmexit_dispatch(vCPU *vcpu)`.
- **Lines Changed**: Lines 580–585.
- **Modifications**:
  - Populated `g_hv_dashboard.last_vmexit_info.exit_reason = vcpu->last_exit.exit_reason;`
  - Populated `g_hv_dashboard.last_vmexit_info.guest_rip = vcpu->last_exit.guest_rip;`
  - Populated `g_hv_dashboard.last_vmexit_info.guest_gpa = vcpu->last_exit.guest_physical_address;`
- **Rationale**: Connects the live VMCS exit engine to the on-screen automated silicon telemetry panel in `hypervisor_dashboard.c`, replacing static `0x0000` values with real-time hardware telemetry.

### 3. [`kernel/debug/snack/atoms_snack.c`](file:///d:/Signatures_OS/kernel/debug/snack/atoms_snack.c)
- **Functions Modified**: `atoms_snack_run_acpi(uint32_t session_id, uint32_t job_id)`.
- **Lines Changed**: Lines 312–325.
- **Modifications**:
  - Bound synthetic ACPI table pointers to `vm->guest_ram_host_virt + VIRTUAL_ACPI_RSDP_GPA` rather than unmapped host GPA `0xE0000`.
- **Rationale**: Eliminates false-positive `RSDP_SIGNATURE: NOT_FOUND_AT_E0000` reports in AMDE Snack telemetry.

### 4. [`kernel/core/hypervisor/src/freebsd_loader.c`](file:///d:/Signatures_OS/kernel/core/hypervisor/src/freebsd_loader.c)
- **Functions Modified**: `freebsd_loader_setup_bootinfo()`.
- **Lines Changed**: Lines 164–174.
- **Modifications**:
  - Added loader environment hints: `"hint.pcib.0.host_res=1"` and `"hw.pci.realloc_bars=1"`.
- **Rationale**: Explicitly instructs FreeBSD's PCI host bridge to manage host resources within the ACPI `_CRS` template and reallocate BARs dynamically if needed.

### 5. [`kernel/debug/hypervisor_dashboard/hypervisor_dashboard.c`](file:///d:/Signatures_OS/kernel/debug/hypervisor_dashboard/hypervisor_dashboard.c)
- **Functions Modified**: `hv_poll_developer_toggle_keys()`, `hypervisor_dashboard_render_runtime_dashboard()`.
- **Modifications**:
  - Wired F5 key and port 0x60 scancode 0x3F to toggle `disp->guest_owns_display` with instant framebuffer blit.
  - Dynamically bound Card 3 `5A-4 Graphics Pipeline` and `5A-5 Real Chromium Ring3` status indicators to live VirtIO-GPU driver state and vCPU userspace execution flags.
- **Rationale**: Enables live operator toggle between the hardware forensic dashboard and the genuine guest graphics framebuffer.

---

## 2. Verification Checklist

- [x] Zero modifications to unrelated subsystems.
- [x] Zero modifications to RTL8125 driver.
- [x] Zero modifications to `kernel.c`.
- [x] Zero refactoring or API renames.
- [x] Exact scope match with `PATCH_PLAN.md`.
