# BOS GPU Driver V7 — Display Manager & Display Infrastructure Architecture

## 1. Executive Overview & Mission Statement

The **BOS Display Manager Subsystem (`kernel/graphics/display/`)** serves as the central, vendor-independent display infrastructure for BOS OS. It replaces ad-hoc vendor-specific display setup with a unified, stateful display pipeline architecture inspired by modern OS display stacks (such as Windows WDDM and Linux DRM/KMS).

Vendor drivers (VMware SVGA II, VirtIO GPU, Intel Native, AMD Radeon, NVIDIA Native) plug into the **Display HAL (`bos_display_hal_ops_t`)**, completely isolating high-level window management, desktop composition, and application graphics from hardware-specific register sets.

---

## 2. Core Architecture & Layering

```
+-----------------------------------------------------------------------+
|  APPLICATIONS & USERLAND GRAPHICS RUNTIME                             |
+-----------------------------------------------------------------------+
                                  |
                                  v
+-----------------------------------------------------------------------+
|  BOIMAGE & DESKTOP ENGINE (Window Manager & UI Subsystem)             |
+-----------------------------------------------------------------------+
                                  |
                                  v
+-----------------------------------------------------------------------+
|  HARDWARE COMPOSITOR & SURFACE PIPELINE                               |
+-----------------------------------------------------------------------+
                                  |
                                  v
+-----------------------------------------------------------------------+
|  GENERIC DISPLAY MANAGER (kernel/graphics/display/)                   |
|  - Connector Manager      - Monitor Manager & EDID 1.4 Parser        |
|  - Mode & Refresh Manager - Multi-Monitor Configuration Engine       |
|  - Plane Pipeline Manager - Atomic State Commit & VBlank Sync Engine  |
+-----------------------------------------------------------------------+
                                  |
                                  v
+-----------------------------------------------------------------------+
|  GPU HAL & VENDOR DRIVERS (bos_gpu_driver_ops_t / bos_display_ops_t)  |
|  [VMware SVGA II] [VirtIO GPU] [Intel HD/Xe] [AMD Radeon] [NVIDIA]    |
+-----------------------------------------------------------------------+
                                  |
                                  v
+-----------------------------------------------------------------------+
|  HARDWARE DISPLAY CONTROLLERS, TRANSCODERS & PHYSICAL MONITORS        |
+-----------------------------------------------------------------------+
```

---

## 3. Subsystem File Layout & Modular Organization

```
kernel/graphics/display/
├── include/
│   ├── bos_display.h             # Core Display Manager Types & Handles
│   ├── bos_edid.h                # EDID 1.4 & CEA-861 Structures
│   ├── bos_connector.h           # Connector Types & State Structs
│   ├── bos_mode.h                # Display Mode & Timing Tables
│   └── bos_atomic.h              # Atomic State Commit Structures
├── manager/
│   ├── display_manager.c         # Display Registration & Central Engine
│   └── display_manager.h
├── connectors/
│   └── connector_manager.c       # VGA, DVI, HDMI, DP, eDP, Virtual Connectors
├── monitors/
│   ├── monitor_manager.c         # Monitor Detection & Hot-Plug Engine
│   └── edid_parser.c             # EDID 1.4 & CEA-861 Extension Parser
├── modes/
│   ├── mode_manager.c            # Resolution Validation & Mode Switching
│   └── refresh_manager.c         # Refresh Rate Controls (24Hz .. 240Hz)
├── multi_monitor/
│   └── multi_monitor.c           # Single, Extended, Clone, Mirror Modes
├── pipeline/
│   ├── display_pipeline.c        # CRTC, Encoder & Connector Routing
│   └── plane_manager.c           # Primary, Cursor, Overlay & Video Planes
├── cursor/
│   └── cursor_manager.c          # Hardware Cursor Position & Surface Controls
├── atomic/
│   ├── atomic_commit.c           # Transactional Atomic Commit Engine
│   └── vblank_manager.c          # VBlank Synchronization & Frame Timings
├── diagnostics/
│   └── display_diagnostics.c     # Forensic Telemetry & EDID Dumps
└── tests/
    └── display_tests.c           # Subsystem Unit Test Suite
```

---

## 4. EDID 1.4 & CEA-861 Extension Parser

The **EDID Parser (`edid_parser.c`)** reads physical monitor descriptors over DDC/I2C:

### Base EDID Structure (128 Bytes)

- **Header**: 8 Bytes (`0x00 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF 0x00`).
- **Vendor & Product ID**: PNP ID (3 Compressed ASCII chars), Product Code, Serial Number, Manufacture Week & Year.
- **EDID Version**: Version `1` / Revision `4`.
- **Basic Display Parameters**: Digital/Analog input flags, Screen dimensions (cm), Gamma value.
- **Color Characteristics**: Chromaticity Coordinates (Rx, Ry, Gx, Gy, Bx, By, Wx, Wy).
- **Established Timings**: Standard VESA modes (640x480@60Hz, 800x600@60Hz, 1024x768@60Hz).
- **Standard Timings**: 8 x 2-byte mode descriptors.
- **Detailed Timing Descriptors (DTD)**: 4 x 18-byte blocks describing Pixel Clock (KHz), HActive, HBlank, VActive, VBlank, HSync Offset/Width, VSync Offset/Width, Physical Display Dimensions (mm).

### CEA-861 Extension Blocks (Tag 0x02)

- **CEA Header**: Revision 3, DTD Byte Offset.
- **Data Block Collection**: Video Data Block (VIC Video Identification Codes), Audio Data Block, Vendor Specific Data Block (VSDB for HDMI/DP).

---

## 5. Atomic State Commit Model

Display configuration changes execute transactionally:

```
                  [REQUEST DISPLAY MODE CHANGE]
                               |
                               v
                +------------------------------+
                | Create New Atomic State Copy |
                +------------------------------+
                               |
                               v
                +------------------------------+
                |  Check State Validity        |
                |  (Clock, Bandwidth, Planes)  |
                +------------------------------+
                     /                    \
              [VALID]                      [INVALID]
                 /                            \
                v                              v
   +-----------------------+      +--------------------------+
   | Hardware Commit Phase |      | Reject & Rollback State  |
   | (Apply CRTC & Planes) |      | (Restore Previous State) |
   +-----------------------+      +--------------------------+
                |
                v
   +-----------------------+
   | Update Active State   |
   +-----------------------+
```

---

## 6. Plane & Pipeline Abstraction

Display hardware exposes modular Planes:
- **Primary Plane**: Main desktop surface (BOImage composition target).
- **Cursor Plane**: Dedicated hardware cursor buffer (64x64 ARGB8888).
- **Overlay Plane**: Secondary UI overlay or video decode passthrough.
- **Video Plane**: Hardware YUV scaling and color space conversion.

---

## 7. Certification & Verification Plan

The Phase 7 implementation includes 12 dedicated Display Manager unit tests expanding the GPU certification suite to **36 total unit tests**:

1. Display Subsystem Enumeration & Manager Init
2. Connector Manager Registration (VGA, DVI, HDMI, DP, eDP, Virtual)
3. EDID Header & PNP Vendor ID Parsing
4. EDID Detailed Timing Descriptor (DTD) Extraction
5. CEA-861 Extension Block Parsing
6. Mode Manager Validation & Preferred Mode Selection
7. Refresh Rate Switching (24Hz - 240Hz)
8. Multi-Monitor Topology Manager (Extend, Clone, Mirror)
9. Hardware Plane Allocation (Primary, Cursor, Overlay)
10. Hardware Cursor Position & Surface Mapping
11. Atomic Commit Engine Check & Rollback
12. VBlank Synchronization & Telemetry Report

**Target**: `Passed = 36, Failed = 0`.
