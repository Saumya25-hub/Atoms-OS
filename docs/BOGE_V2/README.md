# BOS Graphics Engine V2 (BOGE V2) & BOS Surface Presentation Engine (BSPE)
## Official Engineering Architecture & Specification Freeze (Phase 0)

> **Status:** Architecture Frozen (Phase 0 Complete)  
> **Target Systems:** ATOMS OS Kernel & Userspace Graphics Stack  
> **Core Architecture:** Decoupled Rendering (`BOGE V2`) & Presentation (`BSPE`)  
> **Design Parity Targets:** Windows DWM / DXGI, Wayland Compositor, Apple Quartz, Android SurfaceFlinger  

---

## Executive Summary

This documentation suite establishes the **permanent, production-grade graphics architecture** for ATOMS OS. To eliminate immediate-mode rendering bottlenecks, memory bus saturation, and visual tearing, ATOMS OS enforces a strict **Separation of Concerns** across two modular, sibling engines:

1. **BOGE V2 (BOS Graphics Engine V2):** The **Pure Rendering Engine**. Responsible strictly for retained surface caching, font/bitmap texture atlases, drawing primitives, hierarchical damage clipping, and compositing visible window textures into a staging frame.
2. **BSPE (BOS Surface Presentation Engine):** The **Pure Presentation Engine**. Responsible strictly for frame pacing, VSync synchronization, double/triple buffer swap chains, dual-page damage history tracking, hardware cursor plane control, and VRAM page flipping.

---

## Master Documentation Index

| Document ID | Title | Core Subject & Responsibility |
| :---: | :--- | :--- |
| **01** | [Engine Overview](01_ENGINE_OVERVIEW.md) | High-level architecture, separation of concerns (BOGE vs BSPE), relationship with AME and Identity Engine. |
| **02** | [Folder Structure](02_FOLDER_STRUCTURE.md) | Full production directory tree under `kernel/graphics/BOGE/` and `kernel/graphics/BSPE/`. |
| **03** | [Render Pipeline](03_RENDER_PIPELINE.md) | BOGE V2 rendering pipeline: Command Queue → Retained Surface → Render Graph → Blitter → Staging Buffer. |
| **04** | [Presentation Pipeline](04_PRESENTATION_PIPELINE.md) | BSPE presentation pipeline: Present Queue → Frame Pacer → Dual-Page Damage → Partial VRAM Copy → HW Cursor → VSync Flip. |
| **05** | [Render Graph](05_RENDER_GRAPH.md) | Dependency tracking between window layers, clipping calculation, occlusion culling, and batch sorting. |
| **06** | [Resource Manager](06_RESOURCE_MANAGER.md) | Management of surfaces, textures, bitmaps, font atlases, and VRAM memory slabs. |
| **07** | [Display HAL](07_DISPLAY_HAL.md) | Hardware Abstraction Layer separating BSPE from physical Bochs VBE, VGA, VESA, VirtIO, Intel, AMD, and NVIDIA GPU drivers. |
| **08** | [Memory Model](08_MEMORY_MODEL.md) | Strict memory ownership matrix (Who owns what: Surface, Framebuffer, Wallpaper, Fonts, Cursor, VRAM). |
| **09** | [Damage Tracking](09_DAMAGE_TRACKING.md) | Hierarchical region damage trees in BOGE and Dual-Page VRAM Damage History (`Damage(N) ∪ Damage(N-1)`) in BSPE. |
| **10** | [Surface System](10_SURFACE_SYSTEM.md) | Retained `BOGE_Surface` architecture, backing bitmap buffers, dirty flags, and client-side rendering separation. |
| **11** | [Swapchain](11_SWAPCHAIN.md) | Double buffer and Triple buffer swap chain mechanics (Front, Back, Staging buffers, buffer acquisition and release). |
| **12** | [Cursor Pipeline](12_CURSOR_PIPELINE.md) | Decoupled hardware cursor plane (`BSPE_CursorPlane`), VGA/VBE register controls, zero-damage mouse movement. |
| **13** | [Boot Flow](13_BOOT_FLOW.md) | Complete system boot sequence from Kernel → BOHeart → AME → Identity → BOGE Init → BSPE Init → Login UI → Desktop Shell. |
| **14** | [Login Flow](14_LOGIN_FLOW.md) | How the Login Screen renders under BOGE V2 + BSPE without unconditional redraws or laggy mouse movement. |
| **15** | [Window Flow](15_WINDOW_FLOW.md) | Window lifecycle: Creation → Registration → Damage → Clipping → Blitting → Presentation → Destruction. |
| **16** | [Application Render Flow](16_APPLICATION_RENDER_FLOW.md) | How an application draws text or graphics using `BOS_SetText` / `BOS_Update` → Command Queue → Backing Bitmap. |
| **17** | [Frame Lifecycle](17_FRAME_LIFECYCLE.md) | End-to-end timing and state diagram of a 16.67 ms (60 Hz) frame from BOHeart tick to photon emission on screen. |
| **18** | [Thread Model](18_THREAD_MODEL.md) | Concurrency design: Application threads (Producers) vs BOGE Compositor thread (Consumer) vs BSPE Present thread. |
| **19** | [API Reference](19_API_REFERENCE.md) | Complete public API contract for BOGE V2 (`BOGE_*`) and BSPE (`BSPE_*`). |
| **20** | [Implementation Roadmap](20_IMPLEMENTATION_ROADMAP.md) | The definitive 4-Phase Implementation Roadmap: Phase 1 (BSPE), Phase 2 (Surface Cache), Phase 3 (Font Atlas), Phase 4 (HW Cursor). |

---

## Architectural Commandment

> **"No engine shall bypass another. No memory shall have ambiguous ownership. No UI element shall re-rasterize without explicit damage. Rendering shall never block presentation."**
