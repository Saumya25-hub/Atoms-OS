# Third-Party USB Subsystem License Audit

**Document Version**: 1.0  
**Compliance Target**: ATOMS OS Intellectual Property & Open-Source License Policy  
**Audit Scope**: USB Host Controller, Mass Storage Class (MSC), Bulk-Only Transport (BOT), SCSI, and Block Device Bridge  

---

## 1. Executive Summary & Policy Compliance

* **Policy**: Zero GPL / LGPL code allowed in ATOMS OS. All adapted third-party code must be strictly permissively licensed (BSD-2-Clause, BSD-3-Clause, MIT, ISC, Apache 2.0).
* **Attribution**: Original copyrights, licenses, and author notices must be preserved intact in both file headers and dedicated license directories.
* **Separation**: Third-party headers and definitions are placed under [`third_party/usb/`](file:///d:/Signatures_OS/third_party/usb/). All driver, controller, and operating system integration code resides separately under [`kernel/drivers/usb/`](file:///d:/Signatures_OS/kernel/drivers/usb/).

---

## 2. File-Level License Inventory

| Item | File Path | Origin / Upstream Repo | Upstream Path | License | Copyright Notice | Modifications by ATOMS |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **1** | [`third_party/usb/LICENSE`](file:///d:/Signatures_OS/third_party/usb/LICENSE) | FreeBSD Project (`freebsd-src`) | Multiple (`sys/dev/usb/...`) | **BSD-2-Clause** | (c) 1999 MAEKAWA Masahide, (c) 2007-2008 Hans Petter Selasky | License aggregation and attribution statement for ATOMS tree. |
| **2** | [`third_party/usb/include/usb_msc_defs.h`](file:///d:/Signatures_OS/third_party/usb/include/usb_msc_defs.h) | FreeBSD (`freebsd-src`) | `sys/dev/usb/storage/umass.c`<br>`sys/cam/scsi/scsi_all.h` | **BSD-2-Clause** | Copyright (c) 1999 MAEKAWA Masahide<br>Copyright (c) 2007-2008 Hans Petter Selasky | Extracted clean standard USB BOT structures (`usb_msc_cbw_t`, `usb_msc_csw_t`), request opcodes, and SCSI structures (`scsi_inquiry_response_t`, `scsi_read_capacity_10_response_t`) decoupled from FreeBSD kernel macros. |
| **3** | Reference implementation: `sys/dev/usb/usb_subr.c` | OpenBSD (`src`) | `sys/dev/usb/usb_subr.c` (rev 1.135) | **ISC / 2-Clause BSD** | Copyright (c) 1998, 2004 Lennart Augustsson<br>Copyright (c) 2014 Martin Pieuchot | Reference for USB 3.0 SuperSpeed `bMaxPacketSize0` exponent decoding (`mps = 1 << dd->bMaxPacketSize`). |
| **4** | Reference implementation: `sys/dev/usb/xhci.c` | NetBSD (`src`) | `sys/dev/usb/xhci.c` (rev 1.140) | **2-Clause BSD** | Copyright (c) 2013 Takahiro Hayashi | Reference for xHCI Endpoint Context Max Packet Size evaluation and SuperSpeed clamping (`mps = 1 << dd->bMaxPacketSize` -> `512`). |

---

## 3. Subsystem Architecture & Origin Separation

### Third-Party Components (`third_party/usb/`)
* **Standard USB Mass Storage Class Definitions**: Protocol codes, CBW/CSW packet formats, and SCSI-2 / SBC-2 command constants under **BSD-2-Clause**.

### ATOMS-Native Components (`kernel/drivers/usb/` & `kernel/vfs/`)
All procedural execution logic is 100% native ATOMS OS code designed for Haswell xHCI hardware and the ATOMS microkernel:
* **xHCI Bulk Transfer Engine**: [`kernel/drivers/usb/host/xhci/xhci_transfer.c`](file:///d:/Signatures_OS/kernel/drivers/usb/host/xhci/xhci_transfer.c) (Normal TRB generation, doorbell ringing, event ring completion polling, cache flushing).
* **xHCI Endpoint Configuration**: [`kernel/drivers/usb/host/xhci/xhci_transfer.c`](file:///d:/Signatures_OS/kernel/drivers/usb/host/xhci/xhci_transfer.c) (`xhci_configure_bulk_endpoints`).
* **USB MSC Class Driver**: [`kernel/drivers/usb/class/usb_msc.c`](file:///d:/Signatures_OS/kernel/drivers/usb/class/usb_msc.c) (Descriptor matching, binding, LUN querying, BOT state machine).
* **SCSI-to-BOT Bridge**: [`kernel/drivers/usb/class/usb_msc.c`](file:///d:/Signatures_OS/kernel/drivers/usb/class/usb_msc.c) (Executing INQUIRY, READ CAPACITY, READ(10), WRITE(10) with verified CSW reception).
* **ATOMS BlockDevice Bridge**: [`kernel/drivers/usb/class/usb_msc.c`](file:///d:/Signatures_OS/kernel/drivers/usb/class/usb_msc.c) (Connecting MSC SCSI reads directly into the active `BlockDevice` registry).
* **Active Disk Manager & VFS Integration**: [`kernel/vfs/vfs_legacy/storage/src/disk_manager.c`](file:///d:/Signatures_OS/kernel/vfs/vfs_legacy/storage/src/disk_manager.c) (Enumerating USB block devices for MBR partition tables and FAT32 auto-mounting).

---

## 4. Verification Checklist

- [x] Zero GPL / LGPL source files imported.
- [x] File-level audit performed on all upstream references.
- [x] Original copyright notices and BSD-2-Clause text preserved in header.
- [x] Third-party and native code strictly separated into distinct directory hierarchies.
