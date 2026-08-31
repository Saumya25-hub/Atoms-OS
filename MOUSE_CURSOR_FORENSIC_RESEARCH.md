# MOUSE & CURSOR ARCHITECTURE — FORENSIC RESEARCH REPORT

**Focus**: Industry-Standard Cursor Architectures (Windows WDDM/DWM, Linux DRM/KMS, Wayland Compositors) vs. Bare-Metal UEFI GOP Framebuffers  
**Target OS**: ATOMS OS (Haswell LGA1150 H81 Chipset, Native UEFI GOP Linear Framebuffer)  

---

## 1. Industry Research: How Mature Operating Systems Solve Cursor Movement

### 1.1 Windows NT / WDDM & DWM Architecture
In Windows Display Driver Model (WDDM), cursor presentation is architecturally isolated from desktop composition:

```
[Hardware Mouse / HID Interrupt] (125 Hz – 1000 Hz)
            │
            ▼
   [win32kbase.sys / win32k.sys]
            │
            ▼
[DxgkSetPointerPosition / DxgkDdiSetPointerPosition]
            │
            ├────────────────────────────────────────────────┐
            ▼                                                ▼
[Hardware Cursor Plane] (GPU Scanout)        [BasicDisplay.sys / Safe Mode]
- Writes directly to CRTC scanout registers   - Software Cursor Emulation
- Bypasses DWM compositor completely         - Micro-damage tile update (4 KB)
- Zero desktop scene recomposition           - Dual shadow buffer per scanout page
- 0 ms latency / 1000 Hz update rate         - Decoupled from 60 Hz window composition
```

#### Key Architecture Principles in Windows:
1. **Hardware Cursor Plane (Hardware Accelerated Mode)**:
   - When GPU drivers (Intel HD Graphics, NVIDIA, AMD) are loaded, the mouse cursor is never drawn into the desktop backbuffer.
   - The GPU hardware display engine possesses a dedicated **Hardware Cursor Plane** (a hardware-level overlay in silicon).
   - `win32k.sys` sends $(x,y)$ coordinates directly to the GPU display controller registers via `DxgkDdiSetPointerPosition`.
   - The desktop composition engine (DWM) runs at 60 Hz / 120 Hz, but cursor position updates run at the **full hardware mouse polling rate (up to 1000 Hz)** with zero CPU rasterization overhead.
2. **Software Cursor Fallback (Unaccelerated Linear UEFI GOP / BasicDisplay.sys)**:
   - When running on basic UEFI GOP framebuffers without vendor GPU drivers, Windows uses **Software Cursor Emulation with Per-Page Shadow Buffers**:
     - It captures a 32×32 / 64×64 background shadow tile before blitting the cursor sprite.
     - When the cursor moves, it restores the background shadow tile and blits at the new location.
     - Crucially, it executes this micro-transfer **immediately upon input packet arrival**, without triggering a full DWM window tree redraw.

---

### 1.2 Linux DRM/KMS & Wayland Compositor Architecture

```
[evdev / libinput] (1000 Hz)
        │
        ▼
[Wayland Compositor (wlroots / Mutter / KWin)]
        │
        ├──► Hardware Plane Available? ──► YES ──► [DRM_PLANE_TYPE_CURSOR] (Atomic IOCTL -> GPU CRTC)
        │
        └──► Unaccelerated Framebuffer? ──► NO ──► [Software Cursor Fast Path]
                                                    - Tracks `old_box` + `new_box` damage
                                                    - Blits clean background tile from RAM canvas
                                                    - Blits cursor sprite
                                                    - Flushes 4 KB tile over bus (sub-millisecond)
```

#### Key Architecture Principles in Linux:
1. **DRM Atomic Modesetting & Cursor Planes (`DRM_PLANE_TYPE_CURSOR`)**:
   - The Linux kernel Direct Rendering Manager (`drivers/gpu/drm/`) separates display planes into `PRIMARY`, `OVERLAY`, and `CURSOR`.
   - Cursor updates are submitted via `DRM_IOCTL_MODE_CURSOR2` or atomic commits targeting the cursor plane object.
   - The primary framebuffer memory is untouched; the hardware scanout engine overlays the cursor bitmap on the fly.
2. **`simpledrm` / `efifb` Generic UEFI Framebuffer Behavior**:
   - On generic UEFI GOP boot framebuffers (`simpledrm`), the kernel lacks hardware plane registers.
   - The Wayland compositor falls back to **Damage-Driven Software Cursor (`wlr_cursor`)**:
     - The compositor emits damage boxes strictly for the previous cursor rectangle (`old_box`) and current cursor rectangle (`new_box`).
     - It avoids re-evaluating layout or redrawing unmodified application surfaces.
     - The micro-damage blit is executed rapidly (4 KB memory copy) to minimize display scanout collision.

---

## 2. Fundamental Architectural Realities of UEFI GOP Framebuffers

On bare-metal x86 hardware in native UEFI GOP mode (such as the Intel H81 LGA1150 platform):
1. **No Vendor-Specific Hardware Plane Registers via GOP**: The UEFI Graphics Output Protocol (GOP) provides only a single linear base address (`fb_phys_base`), width, height, pitch, and pixel format. It does not provide an API for hardware cursor planes.
2. **Single Scanout Aperture**: The display engine continuously scans out from the physical memory address configured by GOP.
3. **Software Emulation Requirement**: Any OS running on pure UEFI GOP must use a **software-rendered cursor overlay** or **asynchronous micro-damage tile blitter**.
