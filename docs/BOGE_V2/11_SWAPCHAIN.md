# 11. BSPE Swapchain & Buffer Management Specification

> **Module:** BSPE Presentation Engine  
> **Status:** Phase 0 Frozen  
> **Supported Modes:** Double Buffering & Triple Buffering (Async Decoupled)  

---

## 1. Purpose

The BSPE Swapchain governs the circulation, acquisition, and release of video framebuffers between the rendering engine (BOGE V2) and the physical display controller (Display HAL). Its primary responsibility is to eliminate visual tearing while preventing rendering spikes from stalling monitor refresh cycles.

---

## 2. Double vs Triple Buffer Swapchain Architecture

```mermaid
graph TD
    subgraph Double Buffering Mode (Synchronous Pacing)
        D_FRONT[(VRAM Page 0: Front Buffer <br> Active Display Scanline)] -.->|Atomic Page Flip| D_BACK[(VRAM Page 1: Back Buffer <br> Target for BSPE Blit)]
        D_STAGE[(System RAM: Staging Buffer <br> Target for BOGE Blit)] -->|BSPE_PresentFrame| D_BACK
    </subgraph>

    subgraph Triple Buffering Mode (Asynchronous Decoupled Pacing)
        T_FRONT[(VRAM Page 0: Front Buffer <br> Active Display Scanline)] -.->|Atomic Page Flip| T_BACK[(VRAM Page 1: Back Buffer <br> Target for BSPE Blit)]
        T_STAGE1[(System RAM: Staging Buffer A <br> Currently Blitted to VRAM)] -->|BSPE Present Queue| T_BACK
        T_STAGE2[(System RAM: Staging Buffer B <br> FREE for BOGE Frame N+1!)] -->|BOGE Compositor| T_STAGE1
    </subgraph>
```

### 2.1 Why Triple Buffering is Critical for OS Smoothness:
In a Double Buffered system, if BOGE V2 finishes composing Frame $N$ in 10 ms, it must wait 6.67 ms for the monitor VSync pulse before BSPE can flip pages and release the staging buffer. During that 6.67 ms wait, **BOGE V2 is completely blocked from starting Frame $N+1$**.

In **BSPE Triple Buffering Mode**, BSPE maintains an extra staging buffer in system RAM (`Staging Buffer B`). As soon as BOGE V2 submits Frame $N$ on Staging Buffer A, BSPE immediately hands BOGE V2 Staging Buffer B. **BOGE V2 begins composing Frame $N+1$ instantly without waiting for the monitor VSync pulse**, completely decoupling application rendering frame rates from physical monitor refresh rates!

---

## 3. Swapchain Buffer State Machine

Every buffer managed by the BSPE Swapchain cycles through an immutable 4-state lifecycle:

```mermaid
stateDiagram-v2
    [*] --> STATE_FREE: Buffer Allocated in Swapchain Pool
    
    STATE_FREE --> STATE_ACQUIRED_BY_BOGE: BOGE_Swapchain_AcquireBuffer()
    Note over STATE_ACQUIRED_BY_BOGE: BOGE Compositor actively blitting window textures.
    
    STATE_ACQUIRED_BY_BOGE --> STATE_QUEUED_IN_BSPE: BSPE_PresentFrame(handle)
    Note over STATE_QUEUED_IN_BSPE: Waiting in BSPE Present Queue for VSync interval.
    
    STATE_QUEUED_IN_BSPE --> STATE_DISPLAYING_FRONT: VSync IRQ Fires -> Atomic Page Flip!
    Note over STATE_DISPLAYING_FRONT: Actively scanned by physical monitor display controller.
    
    STATE_DISPLAYING_FRONT --> STATE_FREE: Next Frame Flipped -> Buffer Released!
```

---

## 4. Swapchain Data Structure Contract

```c
typedef enum {
    BSPE_BUFFER_STATE_FREE = 0,
    BSPE_BUFFER_STATE_ACQUIRED_BY_BOGE,
    BSPE_BUFFER_STATE_QUEUED_IN_BSPE,
    BSPE_BUFFER_STATE_DISPLAYING_FRONT
} BSPE_BufferState;

typedef struct {
    uint32_t buffer_id;
    void* virtual_address;
    uint32_t physical_address;
    uint32_t pitch;
    uint32_t width;
    uint32_t height;
    BSPE_BufferState state;
    bool is_vram_page; // true for VRAM Page 0/1; false for RAM Staging Buffers
} BSPE_SwapchainBuffer;

typedef struct {
    uint32_t swapchain_id;
    uint32_t buffer_count; // 2 for Double Buffer, 3 for Triple Buffer
    BSPE_SwapchainBuffer buffers[4];
    uint32_t current_front_index;
    uint32_t current_back_index;
    uint32_t vsync_interval; // 1 = 60Hz, 2 = 30Hz
} BSPE_Swapchain;
```

---

## 5. API Contracts & Synchronization

- `BSPE_Swapchain* BSPE_Swapchain_Create(uint32_t width, uint32_t height, uint32_t buffer_count)`
- `BSPE_SwapchainBuffer* BOGE_Swapchain_AcquireBuffer(BSPE_Swapchain* chain)`
- `void BSPE_Swapchain_PresentAndFlip(BSPE_Swapchain* chain, BSPE_SwapchainBuffer* buffer, BSPE_Rect* damage_list, uint32_t damage_count)`
- **Synchronization Rule:** All buffer transitions execute via atomic Compare-And-Swap (CAS) instructions. **Zero mutex locking occurs during buffer acquisition or release.**
