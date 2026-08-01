# SIGNATURES OS — USB PHASE 2: LEGACY HOST CONTROLLER ENGINE (LHCE)
## Technical Specification & Production Architecture Manual

---

## 1. EXECUTIVE SUMMARY

The **Legacy Host Controller Engine (LHCE)** extends the Signatures OS USB Subsystem to support **USB 1.1 (UHCI/OHCI)**, **USB 2.0 (EHCI)**, and seamlessly coexists with the **USB 3.x (xHCI)** Bulk Transfer Engine built in Phase 1. 

With Phase 2 operational, Signatures OS features a unified USB transport architecture capable of addressing host controllers spanning all four generations of the USB specification standard:
- **UHCI:** Universal Host Controller Interface (USB 1.1 Intel/VIA)
- **OHCI:** Open Host Controller Interface (USB 1.1 AMD/Compaq)
- **EHCI:** Enhanced Host Controller Interface (USB 2.0 High-Speed 480 Mbps)
- **xHCI:** eXtensible Host Controller Interface (USB 3.x SuperSpeed 5-10+ Gbps)

---

## 2. DIRECTORY STRUCTURE

```
kernel/
└── usb/
    ├── common/
    │   ├── usb_common.h           # Shared controller types, speeds, endpoint definitions
    │   ├── usb_dma.h              # DMA memory allocation, alignment, bounce buffers
    │   ├── usb_dma.c              # Cache-disabled PMM DMA mapping & flush routines
    │   ├── usb_scheduler.h        # Unified Transfer Queue Manager (Pending/Running/Completed/Timeout)
    │   └── usb_scheduler.c        # Thread-safe request queue operations & watchdog dispatch
    ├── controller/
    │   ├── usb_controller_manager.h  # Master USB Controller Registry & PCI enumeration interface
    │   └── usb_controller_manager.c  # PCI Base Class 0x0C/Subclass 0x03 Prog IF scanner
    ├── uhci/
    │   ├── uhci.h                 # UHCI IO registers, Frame List, TD & QH definitions
    │   └── uhci.c                 # UHCI software reset, frame setup, TD queueing
    ├── ohci/
    │   ├── ohci.h                 # OHCI MMIO registers, HCCA, ED & TD definitions
    │   └── ohci.c                 # OHCI HCCA setup, ED/TD list handling, software reset
    ├── ehci/
    │   ├── ehci.h                 # EHCI capability/op registers, Async QH & qTD definitions
    │   └── ehci.c                 # EHCI Async Circular Schedule, Periodic Frame List, Port Routing
    ├── diagnostics/
    │   ├── lhce_telemetry.h       # Atomic metrics counter definitions
    │   ├── lhce_telemetry.c       # Subsystem telemetry recording routines
    │   ├── lhce_forensic_dump.c   # Detailed register, queue, and DMA inspectors
    │   └── lhce_ai_debug.c        # AI-readable JSON forensic dumper (`usb_dump_everything`)
    ├── tests/
    │   └── lhce_certification_tests.c # 16-Test Phase 2 Certification Suite
    └── docs/
        └── usb_phase2_legacy_host_controller_engine.md # Architecture & manual
```

---

## 3. MASTER CONTROLLER MANAGER & PCI ENUMERATION

The Master USB Controller Manager scans PCI Bus Configuration space for devices matching **Base Class 0x0C (PCI_CLASS_SERIAL_BUS)** and **Subclass 0x03 (PCI_SUBCLASS_USB)**:

| Programming Interface (Prog IF) | Host Controller Standard | Speed Standard | Driver Module |
|---------------------------------|--------------------------|----------------|---------------|
| `0x00`                          | **UHCI**                 | USB 1.1 (Full/Low) | `kernel/usb/uhci/` |
| `0x10`                          | **OHCI**                 | USB 1.1 (Full/Low) | `kernel/usb/ohci/` |
| `0x20`                          | **EHCI**                 | USB 2.0 (High) | `kernel/usb/ehci/` |
| `0x30`                          | **xHCI**                 | USB 3.x (Super) | `kernel/usb/xhci/` |

---

## 4. PUBLIC API REFERENCE

### Controller Manager & PCI Scanning
- `void usb_controller_manager_init(void)`: Initializes controller registry and mutex locks.
- `uint32_t usb_controller_scan_pci(void)`: Scans PCI bus for UHCI/OHCI/EHCI/xHCI controllers and registers active devices.

### UHCI Driver APIs
- `bool uhci_init(uhci_controller_t* udev, uint16_t io_base, uint8_t irq)`: Allocates 1024-entry Frame List and starts processing.
- `bool uhci_reset(uhci_controller_t* udev)`: Executes Host Controller Reset.
- `bool uhci_port_reset(uhci_controller_t* udev, uint8_t port)`: Resets root hub port.

### OHCI Driver APIs
- `bool ohci_init(ohci_controller_t* odev, uint64_t mmio_base, uint8_t irq)`: Allocates 256-byte HCCA area and configures Control/Bulk EDs.
- `bool ohci_reset(ohci_controller_t* odev)`: Software reset for OHCI controller.

### EHCI Driver APIs
- `bool ehci_init(ehci_controller_t* edev, uint64_t mmio_base, uint8_t irq)`: Initializes Async Queue Head circular schedule and Periodic Frame List.
- `bool ehci_port_reset(ehci_controller_t* edev, uint8_t port)`: Issues High-Speed port reset and clears companion owner bit.

### Telemetry & Forensic Diagnostics
- `void usb_dump_controller(uint32_t index)`: Prints register map and status for specified controller.
- `void usb_dump_ports(void)`: Summarizes root hub ports across all registered host controllers.
- `void usb_dump_scheduler(void)`: Displays pending, running, completed, and timeout queues.
- `void usb_dump_dma(void)`: Validates DMA alignment and cache consistency state.
- `void usb_dump_everything(void)`: Emits complete AI-readable JSON diagnostic snapshot.

---

## 5. CERTIFICATION MATRIX (16/16 PASSED)

| Test ID | Test Description | Verification Criteria | Result |
|---------|------------------|-----------------------|--------|
| `TEST 2-01` | PCI Controller Scanning | Detects all PCI USB controllers | **PASS** |
| `TEST 2-02` | UHCI Controller Engine | 1024-entry Frame List & Async Head QH created | **PASS** |
| `TEST 2-03` | OHCI Controller Engine | 256-byte HCCA & Control/Bulk EDs initialized | **PASS** |
| `TEST 2-04` | EHCI Controller Engine | Circular Async Queue Head & Periodic List active | **PASS** |
| `TEST 2-05` | Controller Manager Registry | Multi-controller registry lookup functioning | **PASS** |
| `TEST 2-06` | DMA Engine & Bounce Buffers | 16/64/256/4096-byte alignment & bounce buffers validated | **PASS** |
| `TEST 2-07` | Transfer Queue Scheduler | Pending/Running list enqueue/dequeue atomic operations | **PASS** |
| `TEST 2-08` | Bulk Transfer Enqueue | Bulk TD/qTD descriptor submission across UHCI/OHCI/EHCI | **PASS** |
| `TEST 2-09` | Interrupt Routing & Polling | IRQ status polling and register clearance verified | **PASS** |
| `TEST 2-10` | Root Hub Port Reset | Port reset sequence and line status handling verified | **PASS** |
| `TEST 2-11` | Controller Software Reset | Hardware & register reset recovery verified | **PASS** |
| `TEST 2-12` | Watchdog Timeout Recovery | Timed out request detection and queue state transitions | **PASS** |
| `TEST 2-13` | Hotplug State Machine | Connection state machine & port change detection | **PASS** |
| `TEST 2-14` | AI Diagnostics & JSON Output | Structured JSON diagnostic report generated | **PASS** |
| `TEST 2-15` | Subsystem Telemetry | Atomic submission, completion, & timeout metric recording | **PASS** |
| `TEST 2-16` | Mixed Controller Coexistence | Concurrent operation across UHCI, OHCI, EHCI, and xHCI | **PASS** |

---

## 6. FUTURE INTEGRATION (PHASE 3: USB MASS STORAGE & SCSI ENGINE)

With Phase 1 (xHCI Bulk Transfer Engine) and Phase 2 (Legacy Host Controller Engine) fully operational, the Signatures OS USB Subsystem provides a 100% complete transport protocol layer across all USB versions. 

Phase 3 will build directly upon this foundation to deliver:
1. **SCSI Architecture Engine:** Command Descriptor Blocks (CDB6, CDB10, CDB12, INQUIRY, READ CAPACITY, READ10, WRITE10).
2. **Bulk-Only Transport (BOT) Protocol:** Command Block Wrapper (CBW) and Command Status Wrapper (CSW) transaction state machine.
3. **USB Mass Storage Class Driver (UMS):** Registration into the Signatures OS Block Device Subsystem for FAT32/NTFS mount capability on USB Flash Drives, External HDDs, and SSDs.
