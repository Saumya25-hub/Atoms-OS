# 19. BOGE V2 & BSPE Public API Reference

> **Module:** Public Kernel & Userspace Interface Contract  
> **Status:** Phase 0 Frozen  
> **Target Header Files:** `kernel/graphics/BOGE/include/boge.h` & `kernel/graphics/BSPE/include/bspe.h`  

---

## 1. Purpose

This document provides the definitive API specification for interacting with the BOGE V2 rendering engine and the BSPE presentation engine. Every public function signature, parameter contract, and return value is frozen for Phase 0 architecture parity.

---

## 2. BOGE V2 Core Rendering API (`boge.h`)

### 2.1 Engine Initialization & Lifecycle
```c
/**
 * @brief Initializes the BOGE V2 rendering engine, surface pools, and font atlases.
 * @return true if initialization succeeded; false if kernel heap memory exhausted.
 */
bool BOGE_Initialize(void);

/**
 * @brief Shuts down BOGE V2 and releases all slab memory pools.
 */
void BOGE_Shutdown(void);
```

### 2.2 Retained Surface Management
```c
/**
 * @brief Creates a new retained window surface with a private system RAM backing bitmap.
 * @param width Width in pixels.
 * @param height Height in pixels.
 * @param flags Attribute flags (BOGE_SURFACE_OPAQUE, BOGE_SURFACE_TRANSPARENT).
 * @return Non-zero Surface ID handle; 0 on failure.
 */
uint32_t BOGE_Surface_Create(uint32_t width, uint32_t height, uint32_t flags);

/**
 * @brief Destroys a surface and immediately returns its backing bitmap to the slab pool.
 * @param surface_id Target surface handle.
 */
void BOGE_Surface_Destroy(uint32_t surface_id);

/**
 * @brief Modifies the spatial screen coordinates and dimensions of a surface.
 * @return true if bounds updated successfully.
 */
bool BOGE_Surface_SetBounds(uint32_t surface_id, int32_t x, int32_t y, uint32_t width, uint32_t height);

/**
 * @brief Sets the alpha opacity level for AME fading animations.
 * @param opacity 0x00 (Transparent) to 0xFF (Opaque).
 */
bool BOGE_Surface_SetOpacity(uint32_t surface_id, uint8_t opacity);
```

### 2.3 Asynchronous Drawing Command Submission
```c
/**
 * @brief Submits an asynchronous drawing command packet to a surface's ring buffer.
 * @param surface_id Target surface handle.
 * @param cmd Pointer to populated BOGE_DrawCommand struct.
 * @return true if queued; false if ring buffer is full.
 */
bool BOGE_SubmitDrawCommand(uint32_t surface_id, const BOGE_DrawCommand* cmd);
```

### 2.4 Compositing & Frame Generation
```c
/**
 * @brief Triggers the BOGE V2 compositor to flush queues, build the Render Graph, and blit spans.
 * @return Handle to the completed staging frame ready for BSPE presentation.
 */
BOGE_StagingFrame* BOGE_ComposeFrame(void);
```

---

## 3. BSPE Presentation API (`bspe.h`)

### 3.1 Engine Initialization & Swapchain Control
```c
/**
 * @brief Initializes the BSPE presentation engine, probes Display HAL, and maps VRAM.
 * @param width Display width (e.g. 1024).
 * @param height Display height (e.g. 768).
 * @param buffer_count 2 for Double Buffering, 3 for Triple Buffering.
 * @return true if VRAM mapped and hardware initialized.
 */
bool BSPE_Initialize(uint32_t width, uint32_t height, uint32_t buffer_count);

/**
 * @brief Sets the presentation synchronization interval.
 * @param swap_interval 1 = VSync 60Hz; 0 = Immediate (Tearing allowed for benchmarking).
 */
void BSPE_SetSwapInterval(uint32_t swap_interval);
```

### 3.2 Frame Presentation & Page Flipping
```c
/**
 * @brief Submits a completed staging frame from BOGE V2 to the BSPE Present Queue.
 * @param frame Pointer to staging frame containing buffer handle and damage rectangles.
 * @return true if queued; false if Present Queue is full.
 */
bool BSPE_PresentFrame(const BOGE_StagingFrame* frame);
```

### 3.3 Hardware Cursor Plane Controls
```c
/**
 * @brief Updates the physical mouse cursor coordinates on the hardware sprite plane.
 * @note Executes with zero dirty rectangles and zero VRAM copying!
 * @param x Absolute screen X coordinate.
 * @param y Absolute screen Y coordinate.
 */
void BSPE_Cursor_SetPosition(int32_t x, int32_t y);

/**
 * @brief Uploads a 32x32 ARGB bitmap to the hardware cursor sprite registers.
 * @param argb_32x32 Pointer to 1024-element 32-bit pixel array.
 * @param hotspot_x X offset of pointer tip (0 to 31).
 * @param hotspot_y Y offset of pointer tip (0 to 31).
 */
void BSPE_Cursor_SetImage(const uint32_t* argb_32x32, uint32_t hotspot_x, uint32_t hotspot_y);
```
