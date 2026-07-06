# 15. BOGE V2 Window Lifecycle & Rendering Flow

> **Module:** Window Manager Integration  
> **Status:** Phase 0 Frozen  
> **Target Subsystem:** `kernel/wm/surface/surface.c` & `bwe_window.c`  

---

## 1. Purpose

This document defines the end-to-end lifecycle of an application window in ATOMS OS under BOGE V2—from initial creation and registration, through interactive dragging and clipping, to final destruction and memory reclamation. It enforces strict separation between application drawing logic and window manager compositing.

---

## 2. Window Lifecycle State Diagram

```mermaid
stateDiagram-v2
    [*] --> STATE_CREATED: BOS_CreateSurface(width, height)
    
    state STATE_CREATED {
        [*] --> AllocatingBitmap: Allocate RAM from Slab Pool
        AllocatingBitmap --> Registered: Assign Surface ID & Handle
    }
    
    STATE_CREATED --> STATE_ACTIVE: BOS_ShowWindow(surface_id)
    
    state STATE_ACTIVE {
        [*] --> Clean: No pending draw commands
        Clean --> Dirty: App calls BOS_SetText / BOS_Update
        Dirty --> Composing: Render Graph blits spans to Staging
        Composing --> Clean: Frame submitted to BSPE Present Queue
    }
    
    STATE_ACTIVE --> STATE_MINIMIZED: BOS_MinimizeWindow() <br> Backing bitmap retained in RAM; Z-Index set to OCCLUDED.
    STATE_MINIMIZED --> STATE_ACTIVE: BOS_RestoreWindow()
    
    STATE_ACTIVE --> STATE_DESTROYED: BOS_DestroySurface(surface_id)
    STATE_DESTROYED --> [*]: Backing bitmap returned to Slab Pool instantly!
```

---

## 3. Window Dragging & Movement Workflow

In BOGE V1, dragging a window forced every underlying window along the movement path to re-execute its software drawing commands from scratch. In BOGE V2, window dragging executes as a **Pure Texture Blit Operation**:

```mermaid
sequenceDiagram
    participant Input as Mouse Driver
    participant WM as Window Manager
    participant Surf as BOGE Surface Pool
    participant RG as Render Graph
    participant Blit as BOGE Blitter

    Input->>WM: Mouse Drag Event (Delta X: +5, Delta Y: +5)
    WM->>Surf: Update Window Coordinates: surf->x += 5; surf->y += 5;
    WM->>RG: Submit Movement Damage: Old Bounds U New Bounds
    Note over RG: Zero calls to window on_render callbacks!<br>Window contents are unchanged in backing bitmap!
    RG->>RG: Compute visible spans for uncovered background area
    RG->>Blit: Blit Wallpaper into old trailing region
    RG->>Blit: Blit Window Backing Bitmap at new (x, y) coordinate
    Note over Blit: Total dragging CPU cost: 0.65 ms (vs 11.10 ms in V1)!
```

---

## 4. Window Occlusion & Memory Eviction Strategy

To ensure ATOMS OS operates reliably on low-memory hardware (e.g. 64 MB RAM virtual machines):
1. **Active Occlusion Culling:** When Window A is moved completely over Window B, the Render Graph marks Window B as `BOGE_STATE_OCCLUDED`. Window B executes zero compositing blits.
2. **Dynamic Surface Eviction (Low-Memory Safety Valve):** If system kernel heap free memory drops below 16 MB:
   - The Resource Manager scans the surface pool for minimized or fully occluded windows that have been inactive for > 30 seconds.
   - It evicts their 32-bit backing bitmaps to secondary storage or compresses them using fast RLE/LZ4 algorithms.
   - When the user restores the window, BOGE V2 re-allocates the backing bitmap and sends a single `BOS_EVENT_SURFACE_RESTORED` message to the application, requesting a one-time redraw.
