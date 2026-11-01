# AUDIT PART 4: VIRTIO SUBSYSTEM & STORAGE PIPELINE

## 1. VirtIO Architecture & PCI Device Map

All VirtIO devices in ATOMS implement the **VirtIO 0.9.5 Legacy PCI Interface** [VERIFIED-BY-CODE: kernel/core/hypervisor/src/virtio_pci.c:1-250].
- PCI Vendor ID: 0x1AF4 (Red Hat / VirtIO).
- PCI Subsystem Vendor: 0x1AF4.
- Subsystem Device ID: Equal to VirtIO Subsystem ID.
- Configuration Space: Mechanism #1 (Ports 0xCF8/0xCFC) emulated in irtual_platform.c.
- Base Address Register (BAR 0): 32-byte I/O port region mapped into PCI I/O space.
- Interrupts: PCI PIN A mapped to dedicated GSI lines via ACPI DSDT _PRT table.

| Device Name | PCI BDF | Device ID | Subsystem ID | Virtqueues | Assigned IRQ | Backing Implementation | Status | Evidence |
|:---|:---|:---|:---|:---|:---|:---|:---|:---|
| **VirtIO-Block** | 00:01.0 | 0x1001 | 0x0002 | 1 (q0: size 128) | IRQ 10 (GSI 10) | USB NTFS /FreeBSD_Runtime.img (or RAM chunk table) | ACTIVE | [VERIFIED-BY-CODE: kernel/core/hypervisor/src/virtio_blk.c:220] |
| **VirtIO-Net** | 00:02.0 | 0x1000 | 0x0001 | 2 (RX: q0, TX: q1) | IRQ 11 (GSI 11) | Realtek RTL8168 physical packet ring bridge | ACTIVE | [VERIFIED-BY-CODE: kernel/core/hypervisor/src/virtio_net.c:310] |
| **VirtIO-Input** | 00:03.0 | 0x1052 | 0x0012 | 2 (Event: q0, Status: q1)| IRQ 11 (GSI 11) | Memory event ring | **DEAD CODE** | [VERIFIED-BY-CODE: kernel/core/hypervisor/src/virtio_input.c:1-170] |
| **VirtIO-Display**| 00:04.0 | 0x1050 | 0x0010 | 1 (q0: size 64) | IRQ 11 (GSI 11) | 1024x768 32bpp linear framebuffer at BAR1 | ACTIVE | [VERIFIED-BY-CODE: kernel/core/hypervisor/src/virtio_display.c:1-120] |

---

## 2. Storage Data Path: From Guest Disk Read to Physical USB

`
[1] FreeBSD Guest UFS2 Filesystem (/dev/vtbd0p2)
      | (Issues block read for inode / superblock / data block)
      v
[2] VirtIO-Block Driver (vtbd0 in guest)
      | (Populates virtqueue descriptor chain: Header + Data Buffers + Status Byte)
      | (Writes Queue Index to BAR0 Offset 0x10: QUEUE_NOTIFY)
      v
[3] Hardware VM-Exit Reason 30 (VMX_EXIT_REASON_IO_INSTRUCTION)
      | (Host traps port write at BAR0+0x10)
      v
[4] Host VirtIO Dispatcher (kernel/core/hypervisor/src/virtio_blk.c:virtio_blk_process_queue)
      | (Pops descriptor chain: validates Request Type == VIRTIO_BLK_T_IN)
      | (Extracts start_sector and sector_count)
      v
[5] Runtime Storage Callback (kernel/core/hypervisor/src/runtime_storage.c:runtime_storage_read_cb)
      | (Calculates byte_offset = start_sector * 512, byte_len = sector_count * 512)
      v
[6] NTFS File Driver (kernel/vfs/vfs_legacy/fs/ntfs/src/ntfs.c:ntfs_file_read)
      | (Looks up file extents mapped from Non-Resident  /  runlists)
      | (Translates file byte_offset to partition LBA)
      v
[7] Host Block Device Layer (kernel/vfs/vfs_legacy/storage/src/block_device.c:block_device_read)
      | (Adds partition starting LBA to target sector)
      v
[8] USB Mass Storage BOT Driver (kernel/drivers/usb/class/usb_msc.c:usb_msc_read10)
      | (Constructs 31-byte CBW with SCSI Command 0x28 READ(10))
      | (Calls usb_bulk_transfer for Bulk OUT endpoint)
      | (Calls usb_bulk_transfer for Bulk IN endpoint to receive 512-byte sectors)
      | (Calls usb_bulk_transfer for Bulk IN endpoint to receive 13-byte CSW)
      v
[9] xHCI Controller & Physical USB Flash Drive
      | (Transfers data across USB 3.0 / USB 2.0 bus to host RAM destination)
      v
[10] Host VirtIO Completion & Interrupt Injection
      | (Writes VIRTIO_BLK_S_OK to status byte)
      | (Pushes used ring index and increments vq->last_used_idx)
      | (Asserts virtio_irq_pending = true; raises IOAPIC RTE[10] Vector 0x31)
      v
[11] Guest resumes from VM-Exit with data ready in memory
`

---

## 3. Storage Blocking Behavior, Latency & Failure Modes

### 3.1 Synchronous In-Exit Blocking
- [VERIFIED-BY-CODE: kernel/drivers/usb/class/usb_msc.c:125, 145, 165]
- **Evidence**: usb_bulk_transfer specifies a timeout of 5000 ms (5.0 seconds).
- **Execution Context**: This function is executed **synchronously inside the VM-exit handler** on the Bootstrap Processor (Core 0).
- **Host Freeze Consequence**:
  - If a USB read stalls (e.g. slow NAND flash or sector re-allocation), the Host CPU is blocked in a busy spin-wait loop inside xhci_transfer_bulk.
  - While blocked:
    1. Host BWE desktop rendering halts.
    2. Host USB keyboard/mouse polling halts.
    3. Host Realtek network packet servicing halts.
    4. Host timer ticks continue in hardware, but software dispatch is frozen.

### 3.2 USB Disconnection & Error Handling
- [VERIFIED-BY-CODE: kernel/core/hypervisor/src/runtime_storage.c:70-80]
  `c
  if (bytes_read < 0) {
      g_runtime_storage_telemetry.failed_read_requests++;
      g_runtime_storage_telemetry.last_failed_sector = start_sector;
      return false;
  }
  `
- If the USB drive is unplugged:
  - usb_bulk_transfer returns false after 5000 ms timeout.
  - usb_msc_bot_execute fails and returns false.
  - 
tfs_file_read returns -1.
  - 
untime_storage_read_cb returns false.
  - irtio_blk_process_queue writes VIRTIO_BLK_S_IOERR (0x01) to guest status buffer.
  - Guest FreeBSD UFS2 receives I/O error (EIO). Since root filesystem is read-only, UFS2 panics or prints tbd0: error 5 reading fsbn ....

### 3.3 Strict Read-Only Enforcement
- [VERIFIED-BY-CODE: kernel/core/hypervisor/src/runtime_storage.c:603]:
  VirtIOBlock *blk = virtio_blk_create_file_backed(total_sec, true /* strict read_only */, runtime_storage_read_cb, &s_runtime_image_file);
- [VERIFIED-BY-CODE: kernel/core/hypervisor/src/virtio_blk.c:165-170]:
  case VIRTIO_BLK_T_OUT: if (blk->read_only) { result_status = VIRTIO_BLK_S_IOERR; break; }
- **Result**: Any write attempt by guest FreeBSD to /dev/vtbd0 is rejected at the host VirtIO level with VIRTIO_BLK_S_IOERR without touching the physical USB drive.

---

## 4. Analysis of Dormant VirtIO Devices

### VirtIO-Input (irtio_input.c)
- **Inspection**:
  - Contains full ring buffer event injector (irtio_input_inject_event, irtio_input_send_key, irtio_input_send_mouse_rel).
  - [VERIFIED-BY-CODE: kernel/core/hypervisor/src/virtio_input.c:75-120].
- **Audit Finding**:
  - grep -r virtio_input_send across the entire codebase yields 0 callers outside irtio_input.c and its header.
  - It was registered on PCI bus at 00:03.0, but ATOMS never forwards host USB mouse or keyboard events to it.
  - Guest FreeBSD enumerates it as 
one0@pci0:0:3:0 without a driver.
  - **Verdict**: Completely dormant dead code. Input was routed via virtual i8042 instead.

---

## 5. Top 10 Claims Least Sure About (Part 4)

1. Whether USB 3.0 UASP (USB Attached SCSI Protocol) is supported or if all MSC drives degrade to BOT (Bulk-Only Transport).
2. The exact throughput limit of single-sector (512 bytes) reads versus multi-sector (32 KB) reads across the xHCI ring.
3. Whether 
tfs_file_read handles sparse or compressed NTFS files without corrupting sector offsets.
4. If a guest executes concurrent read requests across multiple descriptor chains, whether virtqueue 0 preserves request ordering.
5. Behavior if guest issues VIRTIO_BLK_T_FLUSH: whether host flushes disk cache or silently returns VIRTIO_BLK_S_OK.
6. Whether Realtek physical NIC driver in irtio_net.c drops multicast Ethernet packets needed for IPv6 neighbor discovery.
7. How much memory is copied during irtio_display_flush() if only a 16x16 pixel mouse cursor area is updated.
8. What happens if a USB MSC drive has 4096-byte physical sectors (Advanced Format 4Kn) instead of 512-byte emulation.
9. Whether disk_manager_get_logical_block_device(0) always maps to the USB drive if a SATA AHCI hard drive is also attached to the motherboard.
10. If guest requests a VirtIO-Block read spanning past the end of the 20 GiB image file boundary, whether 
untime_storage_read_cb zeroes the padding cleanly.
