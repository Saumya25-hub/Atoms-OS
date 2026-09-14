# Chapter 18: Graphics Pipeline & BCM Compositor

ATOMS OS implements a modern, double-buffered graphics pipeline producing 60 FPS visual rendering at 2560x1600 resolution.

## 1. The Composition Pipeline
```
[ Application Client Surface ]
          │
          ▼ (SYS_SHOW_WINDOW / SYS_MAP_SURFACE)
[ BOS Composition Manager (BCM) ]
  • Damage Rectangle Tracking (Dirty Rect Bounding)
  • Alpha-Blending & Window Transparency
  • Z-Order Surface Sorting
  • Wallpaper Layer Integration
          │
          ▼ (60 Hz Hardware TSC / PIT Pacing)
[ Double-Buffered Backbuffer ]
          │
          ▼ (64-bit Burst SSE / VRAM Write-Combining MTRR)
[ UEFI GOP Video RAM Framebuffer ]
```

## 2. Frame Pacing (BSPE)
- Measures hardware TSC elapsed cycles per frame.
- Enforces an exact 16.6 ms frame deadline (60 FPS).
- Utilizes Intel MTRR Write-Combining (WC) attributes on GOP VRAM to maximize PCIe burst write bandwidth.
