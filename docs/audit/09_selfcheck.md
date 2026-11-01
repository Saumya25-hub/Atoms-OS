# ATOMS OS Audit - Part 9: Forensic Self-Check & Fact-Check
**Document**: `docs/audit/09_selfcheck.md`  
**Classification**: READ-ONLY AUDIT - EVIDENCE-BASED  
**Date**: 2026-11-01  
**Scope**: 20 random claims cross-checked directly against disk files, line numbers, and binary artifacts.

---

## 1. Twenty Cross-Checked Audit Claims

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
| **14** | `05_input.md` | Wireless composite receiver ignores secondary mouse interface when Interface 0 is keyboard | `kernel/drivers/usb/class/usb_hid.c:599-604` | **CONFIRMED** | `if (s_usb_kbd_dev != NULL && dev == s_usb_kbd_dev && interface_desc->bInterfaceNumber != 0) return true;` leaving `dev->protocol = 1`. |
| **15** | `05_input.md` | Fallback condition `length == 8 && report[1] == 0` misclassifies mouse reports | `kernel/drivers/usb/class/usb_hid.c:281-283` | **CONFIRMED** | `else if (length == 8 && report[1] == 0) { is_keyboard = true; }` catches 8-byte mouse packets. |
| **16** | `05_input.md` | Virtual i8042 status register lacks AUX bit 5 (`0x20`) | `kernel/core/hypervisor/src/virtual_platform.c:1756-1757` | **CONFIRMED** | Code performs `kbd->status |= 0x01;` only; bit 5 is never asserted anywhere in the function. |
| **17** | `06_history.md` | Commit `e98ace6f` introduced `$ATTRIBUTE_LIST` extent chaining in `runtime_storage.c` | `kernel/core/hypervisor/src/runtime_storage.c:510-522` | **CONFIRMED** | Git log and diff verify commit `e98ace6f` added dynamic `kmalloc` extent concatenation for non-resident `$DATA`. |
| **18** | `07_guest.md` | Embedded kernel in `BOOTX64.EFI` is FreeBSD 14.1-RELEASE | Binary string audit | **CONFIRMED** | Binary contains `@(#)FreeBSD 14.1-RELEASE releng/14.1-n267679-10e31f0946d8` and `1401000`. |
| **19** | `07_guest.md` | FreeBSD 14.5 `init` warning caused by missing `RLIMIT_PIPEBUF = 15` in 14.1 | `/usr/include/sys/resource.h` | **CONFIRMED** | FreeBSD 14.1 defines limits 0..14 (`RLIM_NLIMITS = 15`); FreeBSD 14.5 added `RLIMIT_PIPEBUF = 15` (`RLIM_NLIMITS = 16`). |
| **20** | `08_risks.md` | Fixed runlist extent table size `MAX_EXTENTS = 128` overflow risk | `kernel/core/hypervisor/src/runtime_storage.c:511` | **WRONG, corrected** | Extent list is NOT statically capped at 128; it dynamically allocates `new_exts` via `kmalloc(new_count * sizeof(NTFS_Extent))`. |

---

## 2. Inventory of Unverified Items (Requires Running Silicon Instrument)

The following items could not be verified by static code or offline log analysis alone and require live hardware execution traces:
1. **Raptor Lake P-core vs E-core TSC Rate**: Whether Intel TSC synchronization remains drift-free during deep C-state sleep on Core i3-14100F.
2. **XHCI Bulk-In Maximum Packet Size Babble**: Exact TRB completion status code returned by Intel 700-series PCH XHCI controller during the transfer error (#1127).
3. **FreeBSD `vtnet` In-Flight MTU Fragmentation**: Whether VirtIO-Net drops oversized UDP packets when receiving DHCP answers > 1500 bytes.
4. **Physical RAM Identity Map Collision on Above-4GB GPA**: Whether the UEFI firmware on ASUS PRIME B760M-K maps BAR apertures into the identity-mapped EPT range.
