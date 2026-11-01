# ATOMS OS Audit - Part 9: Forensic Self-Check & Fact-Check
**Document**: `docs/audit/09_selfcheck.md`  
**Classification**: READ-ONLY AUDIT - EVIDENCE-BASED  
**Date**: 2026-11-01  
**Scope**: 40 total claims cross-checked directly against disk files, line numbers, and binary artifacts across two independent sampling rounds.

---

## 1. Round 1 Cross-Checked Claims (Original 20 Claims)

| # | Document | Claim Tested | Referenced File & Line | Binary Verdict | Detailed Verification / Correction |
|---|---|---|---|---|---|
| **1** | `01_map.md` | Path of `hypervisor.c` | `kernel/core/hypervisor/hypervisor.c` | **WRONG, corrected** | Actual disk path is `kernel/core/hypervisor/src/hypervisor.c`. |
| **2** | `01_map.md` | Path of `usb_hid.c` | `kernel/drivers/usb/usb_hid.c` | **WRONG, corrected** | Actual disk path is `kernel/drivers/usb/class/usb_hid.c`. |
| **3** | `02_vmx_core.md` | Host GDT and TR base loaded into guest VMCS fields | `kernel/core/hypervisor/src/hypervisor.c:3232, 3234` | **CONFIRMED** | Code explicitly writes `vmx_vmwrite(VMCS_GUEST_TR_BASE, host_tr_base);` and `vmx_vmwrite(VMCS_GUEST_GDTR_BASE, host_gdtr.base);`. |
| **4** | `02_vmx_core.md` | Interrupt injection priority order: Timer -> KBD -> VirtIO -> UART | `kernel/core/hypervisor/src/hypervisor.c:4012-4022` | **CONFIRMED** | Staging checks `try_inject_lapic_timer` first; if empty, checks `kbd_irq_pending`, then `virtio_irq_pending`, then `uart.rx_irq_pending`. (Line range corrected from 2860 to 4012). |
| **5** | `02_vmx_core.md` | First VM-exit outputs debug banner via COM1 | `kernel/core/hypervisor/src/hypervisor.c:3789-3793` | **CONFIRMED** | Code outputs `>>> VMLAUNCH SUCCESS! <<<` and `>>> FIRST VMEXIT REASON: ` on `exit_count == 1`. |
| **6** | `03_platform.md` | ACPI RSDP placed at GPA `0xE0000` | `kernel/core/hypervisor/src/virtual_platform.c:120` | **CONFIRMED** | `acpi_rsdp_t *rsdp = (acpi_rsdp_t *)(guest_base + VIRTUAL_ACPI_RSDP_GPA);` with header signature `"RSD PTR "`. |
| **7** | `03_platform.md` | DSDT AML bytecode contains PNP0C02 (`SYSR`), PNP0303 (`KBD_`), and PNP0501 (`COM1`) | `kernel/core/hypervisor/src/virtual_platform.c:139-161` | **CONFIRMED** | Bytecode array `s_dsdt_aml` embeds `SYSR` at offset 151, `KBD_` at offset 154, and `COM1` at offset 157. |
| **8** | `03_platform.md` | COM1 UART baud transmitter polling timeout to prevent UDP flooding | `kernel/core/hypervisor/src/virtual_platform.c:376-382` | **CONFIRMED** | Polling loop `uint32_t uart_to = 100000; while ((inb(0x3F8 + 5) & 0x20) == 0 && --uart_to);` protects physical UART. (Line corrected from 1420 to 376). |
| **9** | `03_platform.md` | Port 0xCF9 guest reset sets `VM_STATE_STOPPED` | `kernel/core/hypervisor/src/hypervisor.c:1925-1936` | **WRONG, corrected** | Logic is implemented in `hypervisor.c:1925-1936` inside the I/O exit handler, not in `virtual_platform.c`. |
| **10** | `04_virtio_storage.md` | USB MSC bulk transfer has synchronous 5000 ms timeout | `kernel/drivers/usb/class/usb_msc.c:124-126` | **CONFIRMED** | Parameter 6 to `usb_bulk_transfer` is hardcoded literal `5000`. |
| **11** | `04_virtio_storage.md` | VirtIO-BLK write requests return `VIRTIO_BLK_S_IOERR` in read-only mode | `kernel/core/hypervisor/src/virtio_blk.c:433-437` | **CONFIRMED** | Case `VIRTIO_BLK_T_OUT`: `if (blk->read_only) { result_status = VIRTIO_BLK_S_IOERR; break; }`. (Line corrected from 220 to 436). |
| **12** | `04_virtio_storage.md` | VirtIO-Input device created with 2 virtqueues and device ID `0x1052` | `kernel/core/hypervisor/src/virtio_input.c:41` | **CONFIRMED** | `virtio_device_create(VIRTIO_DEV_ID_INPUT, VIRTIO_PCI_DEVICE_INPUT, 2, ...)` where device ID is `0x1052`. |
| **13** | `04_virtio_storage.md` | `virtio_input_send_key` has zero callers across the OS | `kernel/core/hypervisor/src/virtio_input.c:92` | **CONFIRMED** | Grep of all C/ASM sources confirms 0 callers for `virtio_input_send_key` and `virtio_input_send_mouse_rel`. |
| **14** | `05_input.md` | Non-boot protocol mouse leaves `dev->protocol = 0` | `kernel/drivers/usb/class/usb_hid.c:630` | **CONFIRMED** | When `interface_desc->bInterfaceProtocol == 0`, `dev->protocol = 0`, bypassing mouse protocol check. |
| **15** | `05_input.md` | Fallback condition `length == 8 && report[1] == 0` misclassifies mouse reports | `kernel/drivers/usb/class/usb_hid.c:281-283` | **CONFIRMED** | `else if (length == 8 && report[1] == 0) { is_keyboard = true; }` catches 8-byte mouse packets. |
| **16** | `05_input.md` | Virtual i8042 status register lacks AUX bit 5 (`0x20`) | `kernel/core/hypervisor/src/virtual_platform.c:1756-1757` | **CONFIRMED** | Code performs `kbd->status |= 0x01;` only; bit 5 is never asserted anywhere in the function. |
| **17** | `06_history.md` | Commit `e98ace6f` introduced `$ATTRIBUTE_LIST` extent chaining in `runtime_storage.c` | `kernel/core/hypervisor/src/runtime_storage.c:510-522` | **CONFIRMED** | Git log and diff verify commit `e98ace6f` added dynamic `kmalloc` extent concatenation for non-resident `$DATA`. |
| **18** | `07_guest.md` | Embedded kernel in `BOOTX64.EFI` is FreeBSD 14.1-RELEASE | Binary string audit | **CONFIRMED** | Binary contains `@(#)FreeBSD 14.1-RELEASE releng/14.1-n267679-10e31f0946d8` and `1401000`. |
| **19** | `07_guest.md` | FreeBSD 14.5 `init` warning caused by missing `RLIMIT_PIPEBUF = 15` in 14.1 | `/usr/include/sys/resource.h` | **CONFIRMED** | FreeBSD 14.1 defines limits 0..14 (`RLIM_NLIMITS = 15`); FreeBSD 14.5 added `RLIMIT_PIPEBUF = 15` (`RLIM_NLIMITS = 16`). |
| **20** | `08_risks.md` | Fixed runlist extent table size `MAX_EXTENTS = 128` overflow risk | `kernel/core/hypervisor/src/runtime_storage.c:511` | **WRONG, corrected** | Extent list is NOT statically capped at 128; it dynamically allocates `new_exts` via `kmalloc(new_count * sizeof(NTFS_Extent))`. |

*Round 1 Results*: 16 Confirmed, 4 Corrected (Error Rate: 20%).

---

## 2. Round 2 Cross-Checked Claims (20 NEW Random Claims)

| # | Document | Claim Tested | Referenced File & Line | Binary Verdict | Detailed Verification / Correction |
|---|---|---|---|---|---|
| **21** | `01_map.md` | `freebsd_loader.h` defines `FREEBSD_KERNEL_DEFAULT_ENTRY_GPA` as `0x00200000ULL` | `kernel/core/hypervisor/include/freebsd_loader.h:48` | **CONFIRMED** | Line 48: `#define FREEBSD_KERNEL_DEFAULT_ENTRY_GPA 0x00200000ULL`. |
| **22** | `01_map.md` | Dashboard runtime step passes exit slice count of 500,000 | `kernel/debug/hypervisor_dashboard/hypervisor_dashboard.c:2794` | **WRONG, corrected** | Code executes `atoms_hypervisor_runtime_step(rt_vm, 50);`. Slice is 50 exits, not 500,000 (which was the milestone reporting modulo). |
| **23** | `02_vmx_core.md` | `VMCS_PIN_BASED_VM_EXEC_CONTROL` encoding is `0x00004000` | `kernel/core/hypervisor/include/vmx.h:78` | **CONFIRMED** | Line 78: `#define VMCS_PIN_BASED_VM_EXEC_CONTROL 0x00004000`. |
| **24** | `02_vmx_core.md` | `GUEST_RIP` virtual trial entry point defined as `0xFFFFFFFF8037C000` | `kernel/core/hypervisor/src/hypervisor.c:3586` | **CONFIRMED** | Line 3586: `(trials[r].rip_mode == 0) ? 0x00200000ULL : 0xFFFFFFFF8037C000ULL`. |
| **25** | `02_vmx_core.md` | `GUEST_CR4` default value PAE/PGE/OSFXSR/OSXMMEXCPT is `0x000006A0` | `kernel/core/hypervisor/src/hypervisor.c:3244` | **CONFIRMED** | Line 3244: `uint64_t g_cr4 = vcpu->cr4 ? vcpu->cr4 : 0x000006A0ULL;`. |
| **26** | `02_vmx_core.md` | Exit Reason 10 (`CPUID`) handler advances guest RIP | `kernel/core/hypervisor/src/hypervisor.c:1911` | **CONFIRMED** | Line 1911: `vcpu->guest_regs.rip += vcpu->last_exit.instruction_length;`. |
| **27** | `03_platform.md` | IOAPIC base GPA declared as `0xFEC00000` | `kernel/core/hypervisor/include/virtual_platform.h:37` | **CONFIRMED** | Line 37: `#define VIRTUAL_IOAPIC_BASE_GPA 0xFEC00000ULL`. |
| **28** | `03_platform.md` | Virtual LAPIC base GPA declared as `0xFEE00000` | `kernel/core/hypervisor/include/virtual_platform.h:38` | **CONFIRMED** | Line 38: `#define VIRTUAL_LAPIC_BASE_GPA 0xFEE00000ULL`. |
| **29** | `03_platform.md` | CPUID leaf `0x40000000` returns `"ATOMS OS VMM"` | `kernel/core/hypervisor/src/virtual_platform.c:1164-1168` | **WRONG, corrected** | Leaf returns EBX=`0x4D4F5441` ("ATOM"), ECX=`0x4D565353` ("SSVM"), EDX=0. Signature is `"ATOMSSVM"`. |
| **30** | `04_virtio_storage.md` | VirtIO-BLK configuration space defaults `blk_size=512, seg_max=128, size_max=65536` | `kernel/core/hypervisor/src/virtio_blk.c:213-215` | **CONFIRMED** | `cfg->blk_size = 512; cfg->seg_max = 128; cfg->size_max = 65536;`. |
| **31** | `04_virtio_storage.md` | VirtIO 0.9.5 legacy I/O BAR0 size is 64 bytes (`0xC000 - 0xC03F`) | `kernel/core/hypervisor/src/virtual_platform.c:134, 171` | **CONFIRMED** | PCI BAR0 assigned 64 I/O ports. |
| **32** | `05_input.md` | Scancode translation bit checked via `(command_byte & 0x40)` | `kernel/core/hypervisor/src/virtual_platform.c:1865` | **CONFIRMED** | Line 1865: `bool translate = (plat->i8042.command_byte & 0x40) != 0;`. |
| **33** | `05_input.md` | Focus change calls `atoms_input_release_all_guest_keys()` | `kernel/core/hypervisor/src/virtual_platform.c:1482` | **CONFIRMED** | Line 1482 explicitly executes key release on every focus switch. |
| **34** | `06_history.md` | Commit `fb8f4b03` removed quotation marks from ISA hints | `kernel/core/hypervisor/src/freebsd_loader.c:183-187` | **CONFIRMED** | Diff shows `"hint.atkbdc.0.at=\"isa\""` changed to `"hint.atkbdc.0.at=isa"`. |
| **35** | `06_history.md` | Commit `aa9194b5` modified display refresh pacing to 30fps | Commit message & git log | **CONFIRMED** | Git log confirms 30fps display flush integration. |
| **36** | `07_guest.md` | FreeBSD 14.1 kernel binary integer version is `1401000` | `sys/sys/param.h` / ELF header | **CONFIRMED** | Binary contains integer constant `1401000`. |
| **37** | `07_guest.md` | Minimal rootfs `/sbin/` contains `dhclient` and `ifconfig` | `tools/freebsd_payload/minimal_rootfs/sbin/` | **CONFIRMED** | Files present on disk: `dhclient` (118 KB) and `ifconfig` (324 KB). |
| **38** | `08_risks.md` | Default USB MSC bulk transfer timeout is 5000 ms | `kernel/drivers/usb/class/usb_msc.c:124-126` | **CONFIRMED** | Hardcoded parameter `5000` passed to `usb_bulk_transfer()`. |
| **39** | `08_risks.md` | Virtual UART software log buffer capacity | `kernel/core/hypervisor/include/virtual_platform.h:26` | **WRONG, corrected** | Software log buffer `log_buffer` is already 4096 bytes (`VIRTUAL_UART_LOG_BUFFER_SIZE = 4096`). Only the hardware 16550 FIFO emulation register is 16 bytes. |
| **40** | `08_risks.md` | Rollback target tag `milestone-freebsd-keyboard-ok` commit hash | `git rev-parse milestone-freebsd-keyboard-ok` | **CONFIRMED** | Hash is verified as `3bd4f7e987afd699d0ab4d999830cfa81f04fb0e`. |

*Round 2 Results*: 17 Confirmed, 3 Corrected.  
**Round 2 Error Rate: 15% (3 / 20)**.

---

## 3. Inventory of Items Requiring Live Silicon Traces

1. **Raptor Lake P-core vs E-core TSC Rate**: Verification whether Intel TSC synchronization remains drift-free during deep C-state sleep on Core i3-14100F.
2. **XHCI Bulk-In Maximum Packet Size Babble**: Exact TRB completion status code returned by Intel 700-series PCH XHCI controller during the transfer error (#1127).
3. **FreeBSD `vtnet` In-Flight MTU Fragmentation**: Whether VirtIO-Net drops oversized UDP packets when receiving DHCP answers > 1500 bytes.
4. **Physical RAM Identity Map Collision on Above-4GB GPA**: Whether the UEFI firmware on ASUS PRIME B760M-K maps BAR apertures into the identity-mapped EPT range.
