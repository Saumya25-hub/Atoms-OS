# USB Phase 3: USB Core & URB Engine (UCUE) Architectural Specification

## Overview
The **USB Core & URB Engine (UCUE)** forms the central unified operating system abstraction layer for all USB host controllers and class drivers within **Signatures OS**.

It completely isolates higher-level class drivers (Mass Storage BOT, HID, Audio, Network, Webcam, Bluetooth) from physical host controller implementations (xHCI, EHCI, OHCI, UHCI).

---

## Directory Architecture
```
kernel/usb/
├── core/                       # USB Core Engine & Device State Machine
│   ├── usb_core.h
│   └── usb_core.c
├── urb/                        # Universal Request Block Allocation & Lifecycles
│   ├── usb_urb.h
│   └── usb_urb.c
├── endpoint/                   # Endpoint Registry & Toggle Bit Managers
│   ├── usb_endpoint.h
│   └── usb_endpoint.c
├── pipe/                       # Pipe Manager & Scheduling Handles
│   ├── usb_pipe.h
│   └── usb_pipe.c
├── request/                    # 7-Tier Thread-Safe Priority Request Queues
│   ├── usb_request_queue.h
│   └── usb_request_queue.c
├── dispatcher/                 # Hardware Abstraction Transfer Dispatcher
│   ├── usb_transfer_dispatcher.h
│   └── usb_transfer_dispatcher.c
├── resource/                   # DMA Tracking & Pre-allocated Memory Pools
│   ├── usb_resource_manager.h
│   └── usb_resource_manager.c
├── timeout/                    # Watchdog Timer & Auto-Retry Engine
│   ├── usb_timeout_engine.h
│   └── usb_timeout_engine.c
├── completion/                 # Async/Sync Completion & Callback Dispatcher
│   ├── usb_completion_engine.h
│   └── usb_completion_engine.c
├── diagnostics/                # Structured AI Telemetry & Forensic Dumps
│   ├── ucue_diagnostics.h
│   └── ucue_diagnostics.c
├── tests/                      # 18-Stage UCUE Production Certification Suite
│   └── ucue_certification_tests.c
└── docs/                       # Architectural Specifications & Reports
    └── usb_phase3_usb_core_urb_engine.md
```

---

## Key Subsystems & APIs

### 1. USB Core Engine (`usb_core.h`)
- `usb_core_init()`
- `usb_register_driver()`, `usb_unregister_driver()`
- `usb_register_device()`, `usb_unregister_device()`
- `usb_find_device()`, `usb_get_device_by_address()`
- Device Lifecycle States: `ATTACHED` -> `POWERED` -> `DEFAULT` -> `ADDRESSED` -> `CONFIGURED` -> `SUSPENDED` / `DISCONNECTED`.

### 2. URB Engine (`usb_urb.h`)
- Universal Request Block supporting `Control`, `Bulk`, `Interrupt`, and `Isochronous` transfer types.
- Pre-allocated 512-slot lock-free/spinlock-guarded pool with atomic reference counting.
- APIs: `usb_alloc_urb()`, `usb_free_urb()`, `usb_clone_urb()`, `usb_submit_urb()`, `usb_cancel_urb()`.

### 3. Endpoint & Pipe Managers (`usb_endpoint.h`, `usb_pipe.h`)
- Manages endpoint state transitions (`IDLE`, `RUNNING`, `STALL`, `NAK`, `HALT`, `RESET`).
- Tracks data toggle bit advance/reset (`usb_endpoint_reset_toggle`).
- Generates unique 32-bit pipe handles (`usb_create_pipe`).

### 4. Transfer Dispatcher (`usb_transfer_dispatcher.h`)
- Seamlessly maps URB submissions to target host controllers:
  - **UHCI**: USB 1.1 Intel IO Port Controllers
  - **OHCI**: USB 1.1 AMD/VIA MMIO Controllers
  - **EHCI**: USB 2.0 High-Speed MMIO Controllers
  - **xHCI**: USB 3.x SuperSpeed Controllers

### 5. Request Queue Engine (`usb_request_queue.h`)
- Maintains 7 thread-safe priority queues: `Pending`, `Running`, `Completed`, `Cancelled`, `Timeout`, `Retry`, and `Priority`.

### 6. Diagnostics & Telemetry (`ucue_diagnostics.h`)
- Exports structured JSON reports for AI diagnostic engines (`ucue_telemetry_dump_json`).
- Includes forensic dumps for devices, URBs, endpoints, pipes, queues, resources, and dispatchers.

---

## Certification Suite Matrix
- **TEST 3-01**: USB Core Initialization (PASS)
- **TEST 3-02**: Driver Registration (PASS)
- **TEST 3-03**: Device Registration & State Machine (PASS)
- **TEST 3-04**: URB Allocation, Cloning & Refcounting (PASS)
- **TEST 3-05**: URB Completion & Callback Pipeline (PASS)
- **TEST 3-06**: Endpoint Manager (PASS)
- **TEST 3-07**: Pipe Manager (PASS)
- **TEST 3-08**: Request Queue Engine (PASS)
- **TEST 3-09**: Transfer Dispatcher (PASS)
- **TEST 3-10**: Device State Machine Transitions (PASS)
- **TEST 3-11**: Timeout Engine (PASS)
- **TEST 3-12**: Completion Engine (PASS)
- **TEST 3-13**: Error Recovery Engine (PASS)
- **TEST 3-14**: Resource Manager (PASS)
- **TEST 3-15**: AI Forensic Diagnostics (PASS)
- **TEST 3-16**: Telemetry Validation Engine (PASS)
- **TEST 3-17**: Multi-Controller Dispatch (PASS)
- **TEST 3-18**: High Load Stress Test - 10,000 URBs (PASS)

---

## Preparation for Phase 4
With Phase 3 complete, **Signatures OS** has a unified USB Core & URB Engine. Phase 4 will implement **USB Mass Storage (BOT — Bulk-Only Transport) & SCSI Command Engine**, allowing real USB flash drives to mount and exchange SCSI commands with the OS VFS.
