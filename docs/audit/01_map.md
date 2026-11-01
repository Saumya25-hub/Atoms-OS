# AUDIT PART 1: SYSTEM MAP & TIMELINE

## 1. Directory Tree & Component Inventory

### 1.1 Hypervisor Subsystem (kernel/core/hypervisor)

#### Headers (include/)
- hypervisor.h (7,037 bytes) [VERIFIED-BY-CODE: kernel/core/hypervisor/include/hypervisor.h:1-200]
  - **Purpose**: Defines VirtualMachine, CPU, VM exit reasons, VM states (VM_STATE_CREATED, VM_STATE_RUNNING, etc.), and public lifecycle API (toms_hypervisor_boot_genuine_freebsd, toms_hypervisor_runtime_step).
  - **Caller**: hypervisor.c, hypervisor_dashboard.c, kernel.c.
- irtual_platform.h (13,304 bytes) [VERIFIED-BY-CODE: kernel/core/hypervisor/include/virtual_platform.h:1-350]
  - **Purpose**: Defines chipset emulation structs: VirtualPlatform, VirtualI8042, VirtualUART, VirtualIOAPIC, VirtualPIC, VirtualPIT, VirtualRTC, PCI bus, and input focus state enum (INPUT_FOCUS_HOST, INPUT_FOCUS_GUEST).
  - **Caller**: irtual_platform.c, hypervisor.c, usb_hid.c.
- mx.h (8,742 bytes) [VERIFIED-BY-CODE: kernel/core/hypervisor/include/vmx.h:1-250]
  - **Purpose**: Intel VMX VMCS encoding definitions (guest/host state, execution controls, pin/proc controls, exit reasons per Intel SDM Volume 3C Chapter 24).
  - **Caller**: hypervisor.c.
- ept.h (3,823 bytes) [VERIFIED-BY-CODE: kernel/core/hypervisor/include/ept.h:1-120]
  - **Purpose**: Extended Page Table (EPT) 4-level paging structures, entry bit definitions (Read, Write, Execute, Memory type, 2MB large page flag).
  - **Caller**: ept.c, hypervisor.c.
- reebsd_loader.h (6,490 bytes) [VERIFIED-BY-CODE: kernel/core/hypervisor/include/freebsd_loader.h:1-180]
  - **Purpose**: FreeBSD bootinfo, metadata module tags (MODINFO_NAME, MODINFO_TYPE, MODINFO_METADATA), ELF64 header types, and kernel loader entrypoints.
  - **Caller**: reebsd_loader.c, hypervisor.c.
- 
untime_storage.h (1,927 bytes) [VERIFIED-BY-CODE: kernel/core/hypervisor/include/runtime_storage.h:1-60]
  - **Purpose**: Declarations for 
untime_storage_init(), 
untime_storage_is_available(), 
untime_storage_create_guest_blk(), and RuntimeStorageTelemetry.
  - **Caller**: 
untime_storage.c, hypervisor.c, hypervisor_dashboard.c.
- irtio_blk.h (3,496 bytes) [VERIFIED-BY-CODE: kernel/core/hypervisor/include/virtio_blk.h:1-110]
  - **Purpose**: VirtIO-Block device struct VirtIOBlock, request headers (irtio_blk_req_hdr_t), status bytes, and sector transfer function pointers.
  - **Caller**: irtio_blk.c, hypervisor.c, 
untime_storage.c.
- irtio_net.h (5,750 bytes) [VERIFIED-BY-CODE: kernel/core/hypervisor/include/virtio_net.h:1-160]
  - **Purpose**: VirtIO-Network device struct VirtIONet, packet buffer ring, and physical NIC packet forwarding hooks.
  - **Caller**: irtio_net.c, hypervisor.c.
- irtio_display.h (6,302 bytes) [VERIFIED-BY-CODE: kernel/core/hypervisor/include/virtio_display.h:1-180]
  - **Purpose**: Shared framebuffer device (VirtIODisplay), resolution config (1024x768x32bpp), guest dirty rect tracking, and host BWE composition blit.
  - **Caller**: irtio_display.c, hypervisor.c, hypervisor_dashboard.c.
- irtio_pci.h (2,618 bytes), irtio_device.h (2,102 bytes), irtio_queue.h (2,520 bytes), irtio_types.h (7,085 bytes) [VERIFIED-BY-CODE]
  - **Purpose**: Base VirtIO 0.9.5 (legacy PCI) register interface, BAR0 layout, queue descriptor table, available ring, used ring manipulation.
- toms_bridge.h (1,396 bytes) [VERIFIED-BY-CODE: kernel/core/hypervisor/include/atoms_bridge.h:1-40]
  - **Purpose**: Interface between host BWE/Desktop and guest display surfaces.

#### Source Files (src/)
- hypervisor.c (219,904 bytes) [VERIFIED-BY-CODE: kernel/core/hypervisor/src/hypervisor.c:1-4550]
  - **Purpose**: Core VMX implementation. Contains VMXON activation, VMCS region allocation and field population, host/guest register state setup, VM-exit dispatcher loop, I/O instruction emulation, CPUID/MSR virtualization, interrupt window injection, and guest vCPU step execution (toms_hypervisor_runtime_step).
  - **Main Functions**: mx_init(), irtual_machine_create(), toms_vm_launch(), toms_hypervisor_runtime_step(), mx_exit_handler(), handle_io_instruction(), handle_cr_access().
  - **Who Calls It**: kernel.c (via dashboard), hypervisor_dashboard.c.
- irtual_platform.c (85,086 bytes) [VERIFIED-BY-CODE: kernel/core/hypervisor/src/virtual_platform.c:1-2050]
  - **Purpose**: Complete IBM PC/AT chipset and platform peripheral virtualization. Contains virtual 8259 dual-PIC, virtual LAPIC, virtual IOAPIC, virtual i8042 keyboard/mouse controller, virtual 16550 UART (COM1 0x3F8), virtual PIT 8254, virtual RTC, virtual PCI bus (0xCF8/0xCFC), and the unified Input Focus Router (toms_input_forward_hid_to_i8042, toms_input_set_focus).
  - **Main Functions**: irtual_platform_create(), irtual_platform_emulate_io(), irtual_i8042_push_byte(), irtual_i8042_pop_byte(), toms_input_forward_hid_to_i8042(), irtual_ioapic_read(), irtual_ioapic_write().
  - **Who Calls It**: hypervisor.c (on Port I/O and MMIO exits), usb_hid.c (input router).
- reebsd_loader.c (24,147 bytes) [VERIFIED-BY-CODE: kernel/core/hypervisor/src/freebsd_loader.c:1-560]
  - **Purpose**: Direct-kernel bootloader for FreeBSD ELF64 kernel. Constructs 64-bit page tables (PML4 at GPA 0x20000), populates FreeBSD ootinfo_t (GPA 0x11000) and preload_metadata stream (GPA 0x12000), sets loader environment variables (fs.root.mountfrom, init_script, console=vidconsole), and loads kernel ELF segments.
  - **Main Functions**: reebsd_loader_load_kernel(), reebsd_loader_setup_bootinfo(), reebsd_loader_setup_guest_paging(), reebsd_loader_init_root_disk().
  - **Who Calls It**: hypervisor.c (toms_hypervisor_boot_genuine_freebsd).
- 
untime_storage.c (34,120 bytes) [VERIFIED-BY-CODE: kernel/core/hypervisor/src/runtime_storage.c:1-850]
  - **Purpose**: Physical USB NTFS virtual disk backend. Scans USB MSC block devices, parses GPT protective MBR and GPT partition headers, locates NTFS partition, mounts NTFS volume, parses MFT records and Non-Resident $ATTRIBUTE_LIST / $DATA runlists for /FreeBSD_Runtime.img, and exposes file extents to VirtIO-Block via 
untime_storage_read_cb.
  - **Main Functions**: 
untime_storage_init(), 
untime_storage_create_guest_blk(), 
untime_storage_read_cb().
  - **Who Calls It**: hypervisor.c (during irtual_machine_create).
- irtio_blk.c (19,926 bytes) [VERIFIED-BY-CODE: kernel/core/hypervisor/src/virtio_blk.c:1-490]
  - **Purpose**: VirtIO-Block PCI device (Device ID 0x1001). Processes virtqueue 0 descriptor chains, decodes VIRTIO_BLK_T_IN (read) and VIRTIO_BLK_T_OUT (write), executes disk reads via RAM-disk chunks or file-backed callback (lk->read_fn), asserts PCI IRQ Line 10 (GSI 10), and raises IOAPIC interrupt.
  - **Main Functions**: irtio_blk_create(), irtio_blk_process_queue(), irtio_blk_read_sectors().
  - **Who Calls It**: hypervisor.c, 
untime_storage.c.
- irtio_net.c (39,034 bytes) [VERIFIED-BY-CODE: kernel/core/hypervisor/src/virtio_net.c:1-950]
  - **Purpose**: VirtIO-Net PCI device (Device ID 0x1000). Implements RX/TX virtqueues, connects guest network packets to host Realtek RTL8168/8111 or Intel NIC via raw packet rings.
  - **Who Calls It**: hypervisor.c.
- irtio_display.c (14,484 bytes) [VERIFIED-BY-CODE: kernel/core/hypervisor/src/virtio_display.c:1-350]
  - **Purpose**: VirtIO-Display device (Device ID 0x1050). Provides 1024x768 32bpp linear guest framebuffer, tracks dirty rectangular bounds, flushes to host display/BWE surface.
  - **Who Calls It**: hypervisor.c, hypervisor_dashboard.c.
- irtio_pci.c (14,547 bytes), irtio_queue.c (7,006 bytes), irtio_device.c (5,115 bytes) [VERIFIED-BY-CODE]
  - **Purpose**: Standard VirtIO 0.9.5 transport layer, queue indexing, ring wrapping logic.
- ept.c (11,136 bytes), guest_memory.c (11,114 bytes) [VERIFIED-BY-CODE]
  - **Purpose**: Guest physical address (GPA) to host physical address (HPA) mapping using 2MB large pages and 4KB base pages. Identity-maps 0 to 2048 MB.
- mx_entry.asm (4,969 bytes) [VERIFIED-BY-CODE: kernel/core/hypervisor/src/vmx_entry.asm:1-120]
  - **Purpose**: Pure assembly hardware transition. Defines mx_launch_asm and mx_resume_asm. Saves host general registers, executes mlaunch / mresume, handles hardware VM-exit, saves guest registers to vCPU struct, restores host registers, and returns exit reason to C dispatcher.

---

### 1.2 USB Subsystem (kernel/drivers/usb)
- class/usb_hid.c (31,953 bytes) [VERIFIED-BY-CODE: kernel/drivers/usb/class/usb_hid.c:1-780]
  - **Purpose**: USB Human Interface Device class driver. Binds keyboard and mouse interfaces, processes raw interrupt IN reports in usb_hid_report_received, implements chord detection (Ctrl+Alt+G, F5), and routes input between Host desktop and Guest i8042.
  - **Main Functions**: usb_hid_report_received(), usb_hid_bind(), usb_hid_set_leds_ex().
  - **Who Calls It**: xhci_transfer.c (on interrupt completion).
- class/usb_msc.c (20,681 bytes) [VERIFIED-BY-CODE: kernel/drivers/usb/class/usb_msc.c:1-550]
  - **Purpose**: USB Mass Storage Class Bulk-Only Transport (BOT) and SCSI-2 command set (READ(10), WRITE(10), INQUIRY, READ_CAPACITY(10)).
  - **Main Functions**: usb_msc_bot_transfer(), usb_msc_read10(), usb_msc_read_capacity().
  - **Who Calls It**: lock_device.c, 
untime_storage.c.
- host/xhci/xhci.c (19,336 bytes), xhci_transfer.c (28,488 bytes), xhci_cmd.c (11,072 bytes), xhci_ring.c (3,075 bytes) [VERIFIED-BY-CODE]
  - **Purpose**: Intel/ASMedia/NEC USB 3.0 Extensible Host Controller Interface (xHCI) hardware driver. Manages DCBAA, command rings, event rings, transfer rings, scratchpad buffers, and root hub port routing.
  - **Main Functions**: xhci_init(), xhci_poll(), xhci_transfer_bulk(), xhci_transfer_interrupt().
  - **Who Calls It**: kernel.c, hypervisor_dashboard.c (xhci_poll).

---

### 1.3 Debug Subsystem (kernel/debug)
- hypervisor_dashboard/hypervisor_dashboard.c (105,820 bytes) [VERIFIED-BY-CODE: kernel/debug/hypervisor_dashboard/hypervisor_dashboard.c:1-2950]
  - **Purpose**: ATOMS Matrix Debug Engine (AMDE) / Hypervisor Dashboard. Renders 28-stage diagnostic tables, telemetry bars, VMCS register dumps, live key latency counters, and hosts the cooperative runtime loop hypervisor_dashboard_run().
  - **Main Functions**: hypervisor_dashboard_init(), hypervisor_dashboard_run(), hypervisor_dashboard_render_frame(), hv_poll_developer_toggle_keys().
  - **Who Calls It**: kernel.c:887.

---

### 1.4 Tools & Payloads (	ools/)
- 	ools/freebsd_payload/freebsd_stripped_kernel.elf (28,774,400 bytes) [VERIFIED-BY-CODE: tools/freebsd_payload/freebsd_stripped_kernel.elf]
  - **Purpose**: Stripped FreeBSD 14.1-RELEASE amd64 GENERIC ELF kernel binary. Embedded into BOOTX64.EFI via kernel/embedded_freebsd_elf.asm.
- 	ools/make_minimal_ufs2.py (19,418 bytes) [VERIFIED-BY-CODE: tools/make_minimal_ufs2.py:1-450]
  - **Purpose**: Generates 16 MB minimal FreeBSD UFS2 disk image (uild/media.img) containing /etc/rc stage D0 diagnostic runner.
- 	ools/pxe_server.py (22,852 bytes) [VERIFIED-BY-CODE: tools/pxe_server.py:1-600]
  - **Purpose**: Host-side TFTP, DHCP, and UDP telemetry logging server for PXE network booting.

---

## 2. Boot-to-Guest Execution Timeline

The complete sequential control path on physical hardware:

`
[1] UEFI Firmware (BIOS)
    -> Hands off control to BOOTX64.EFI (boot/uefi/bootx64.c:efi_main)
    [VERIFIED-BY-CODE: boot/uefi/bootx64.c:45-300]

[2] UEFI Bootloader Pre-Flight (BOOTX64.EFI)
    -> Acquires physical memory map via GetMemoryMap()
    -> Locates ACPI RSDP (0xE0000 / EFI Configuration Table)
    -> Downloads media.img (16MB fallback) via TFTP or disk into ramdisk_base
    -> Exits UEFI Boot Services (ExitBootServices)
    -> Jumps to 64-bit kernel_main(boot_info)
    [VERIFIED-BY-CODE: boot/uefi/bootx64.c:265-295]

[3] BOS Kernel Initialization (kernel/kernel.c:kernel_main)
    -> Memory subsystem init (PMM, VMM)
    -> Core GDT, IDT, LAPIC, IOAPIC bring-up
    -> PCI bus scan and enumeration
    -> USB subsystem init:
         usb_registry_init() -> usb_hid_init() -> usb_msc_init() -> xhci_init()
    [VERIFIED-BY-CODE: kernel/kernel.c:602-607]

[4] Hypervisor Launch Trigger (kernel/kernel.c:885-888)
    -> block_device_init()
    -> ntfs_init()
    -> hypervisor_dashboard_run(boot_info)
    [VERIFIED-BY-CODE: kernel/kernel.c:884-888]

[5] Hypervisor Dashboard Stage Pipeline (hypervisor_dashboard_run_stage_pipeline)
    -> 28 verification stages executed sequentially:
         Stage 1: CPU hardware VMX capability check (IA32_VMX_BASIC MSR)
         Stage 2: VMXON region setup and CR4.VMXE enable
         Stage 3: Memory allocator verification
         ...
         Stage 18: FreeBSD ELF kernel loading into guest RAM (GPA 0x400000)
         Stage 19: Preload metadata generation (GPA 0x12000)
         Stage 20: 64-bit guest paging tables (GPA 0x20000)
         Stage 21: Pre-flight VMCS consistency check
         Stage 22: atoms_vm_launch() executes initial VMLAUNCH
    [VERIFIED-BY-CODE: kernel/core/hypervisor/src/hypervisor.c:4300-4550]
    [VERIFIED-BY-LOG: build/atoms_live_kernel.log #11004-#11283]

[6] Storage & Device Setup in VM Creation (hypervisor.c:virtual_machine_create)
    -> runtime_storage_init() scans USB devices:
         GPT protective MBR checked -> GPT partition verified -> NTFS mounted
         Resolves /FreeBSD_Runtime.img (20 GiB)
         Reads Non-Resident  and maps data extents
         Creates VirtIO-Block device backed by USB runtime image (vtbd0)
    -> VirtIO-Net, VirtIO-Display, VirtIO-Input, Virtual Platform (i8042, UART, APIC) attached
    [VERIFIED-BY-CODE: kernel/core/hypervisor/src/hypervisor.c:350-395]
    [VERIFIED-BY-LOG: build/atoms_live_kernel.log #10876-#10878]

[7] Hardware Guest Entry (vmx_entry.asm:vmx_launch_asm)
    -> vmlaunch instruction executed
    -> CPU transitions into Guest Non-Root Long Mode at RIP 0xFFFFFFFF8037C000 (btext)
    [VERIFIED-BY-CODE: kernel/core/hypervisor/src/vmx_entry.asm:1-40]
    [VERIFIED-BY-LOG: build/atoms_live_kernel.log #11279-#11351]

[8] Persistent Runtime Cooperative Loop (hypervisor_dashboard.c:hypervisor_dashboard_run)
    -> while (1) infinite loop interleaves:
         1. hv_poll_developer_toggle_keys() -> xhci_poll() (USB HID + MSC events)
         2. atoms_hypervisor_runtime_step(rt_vm, 50) (Guest vCPU execution slice)
         3. virtio_display_flush() (30 fps guest framebuffer blit to display)
         4. net_poll() (Realtek NIC receive ring)
         5. Screenshot & diagnostic telemetry rendering
    [VERIFIED-BY-CODE: kernel/debug/hypervisor_dashboard/hypervisor_dashboard.c:2770-2880]
`

---

## 3. Concurrency & Interleaving Architecture

### 3.1 Single-Threaded Cooperative Scheduling on BSP (Core 0)
- **Code Inspection Fact**: ATOMS OS has an SMP kernel and scheduler, but when running in ATOMS_DEBUG_MODE_HYPERVISOR_DASHBOARD, the hypervisor does NOT run as an independent preemptive preemptible kernel thread.
- [VERIFIED-BY-CODE: kernel/kernel.c:690, 887]: hypervisor_dashboard_run(boot_info) is called directly on the bootstrap core stack and NEVER returns (while(1) loop at hypervisor_dashboard.c:2770).
- **Who runs when**:
  1. xhci_poll() runs first: reads xHCI transfer/event rings. If a USB keyboard key was pressed or a BOT transfer finished, it updates driver queues.
  2. toms_hypervisor_runtime_step(rt_vm, 50) runs second: calls mresume in a loop for up to 50 exits OR until 250 microseconds elapse [VERIFIED-BY-CODE: kernel/core/hypervisor/src/hypervisor.c:4030-4050].
  3. irtio_display_flush() runs third: checks if 33 ms elapsed (~30 fps) and blits the 1024x768 framebuffer.
  4. 
et_poll() runs fourth: services the Ethernet NIC RX ring.
- **Consequence**:
  - The guest vCPU, USB driver, display renderer, and network stack share a SINGLE CPU core sequentially.
  - If a USB BOT read inside a VM-exit blocks waiting for xHCI, the entire host hangs until that read completes or times out.

---

## 4. Top 10 Claims Least Sure About (Part 1)

1. Whether AP cores are genuinely asleep in mwait/hlt or completely uninitialized during hypervisor_dashboard_run.
2. Whether g_embedded_freebsd_elf in .rodata is completely bypassed when loading from USB or if it is still partially touched during stage 18.
3. The exact maximum execution duration inside toms_hypervisor_runtime_step when the guest executes non-exiting tight loops (e.g. without timer interrupts).
4. Whether irtio_display_flush() tears or causes micro-stutter when blitting 3MB (1024x768x4) while guest is actively writing to VRAM.
5. Whether 
et_poll() drops UDP telemetry packets if the guest vCPU consumes more than 2 ms in a single step.
6. The exact behavior of xHCI interrupt transfer ring when multiple HID devices (keyboard + mouse) fire simultaneously within one xhci_poll().
7. Whether reebsd_loader_init_root_disk() completely skips ramdisk binding when m->blk_dev->read_fn != NULL on all code paths.
8. Whether BWE (Bare-Metal Window Engine) compositor task is completely inactive during hypervisor dashboard mode.
9. How much physical RAM PMM actually has left after allocating 2048 MB for guest RAM and 200 MB for NTFS buffers on an 8 GB machine.
10. Whether BOOTX64.EFI PXE TFTP transfer code supports files larger than 32 MB without fragmentation/timeout.
