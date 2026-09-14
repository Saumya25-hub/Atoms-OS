# ATOMS OS — Phase M4: Universal Hardware Video Acceleration Forensic Audit
**Document ID**: `docs/media/GPU_VIDEO_ACCELERATION_FORENSIC_AUDIT.md`  
**Protocol Stage**: Task 1 (Forensic Investigation & Architecture Specification)  
**Subsystems**: BOS Video Acceleration HAL, Vendor Hardware Backends (Intel, NVIDIA, AMD), GPU Surface Architecture, Asynchronous Decode & Synchronization  
**Author**: ATOMS OS Architecture Authority  
**Date**: September 10, 2026  
**Status**: FORENSIC AUDIT COMPLETE & SPECIFICATION LOCKED  

---

## 1. Executive Summary & Mission Statement

### 1.1 Corrective Redirection
Previous early implementations treated hardware video acceleration as an isolated, discrete check (e.g., hardcoded detection of an NVIDIA RTX 4060 reporting `NOT IMPLEMENTED -> software fallback`).

**That model is formally superseded and rejected.**

The mission of ATOMS OS Phase M4 is to establish a **Universal Hardware Video Acceleration Architecture**. ATOMS OS must discover, negotiate, configure, submit bitstream workloads to, synchronize with, and extract decoded surfaces from whatever hardware video acceleration engines are physically available on the target platform:

```
                      BOSpectra Media Pipeline
                                 |
                                 v
                    BOS Video Acceleration HAL
                                 |
           +---------------------+---------------------+
           |                     |                     |
           v                     v                     v
    Intel Backend         NVIDIA Backend          AMD Backend
   (Gen7-Gen12/Xe)           (NVDEC)             (VCN / DCN)
     [Intel VDBox]       [NVIDIA NVDEC Engine]  [AMD VCN Engine]
           |                     |                     |
           +---------------------+---------------------+
                                 |
                                 v
                       Hardware Video Decoder
                                 |
                                 v
                         GPU Video Surface
                         (NV12 / P010 / HW)
                                 |
                                 v
                         BOSpectra Renderer
                                 |
                                 v
                          BOSurface v2.5
```

### 1.2 Architectural Invariants
1. **Vendor Neutrality**: The common BOS Video Acceleration HAL contains zero vendor-specific terminology, register offsets, or assumptions.
2. **True Hardware Acceleration**: Hardware acceleration is achieved only when the GPU engine processes compressed bitstream slices and outputs to hardware video surfaces without synchronous CPU round-trip copies.
3. **Transparent Fallback**: When hardware acceleration is unavailable or a stream's profile/resolution exceeds hardware capabilities, the system falls back to the BOS C99 software decoders (H.264, HEVC, VP8, VP9) and logs the exact forensic cause.
4. **License Integrity**: Zero GPL code is imported into ATOMS OS. Open-source vendor stacks (Intel media-driver, NVIDIA open-gpu-kernel-modules, AMD rocDecode) are studied as engineering references; native implementations use permissible or clean-room BOS-native code.

---

## 2. Forensic Study 1: Intel Hardware Video Architecture

### 2.1 Reference Sources
- **Intel media-driver (`iHD`)**: Modern open-source user-space video driver (MIT License, `https://github.com/intel/media-driver`).
- **Intel Linux Kernel Graphics Driver (`i915` / `xe`)**: Hardware register documentation, VDBox ring submission, and GTT/GGTT mapping.
- **Intel PRMs (Programmer's Reference Manuals)**: Gen7 (Haswell) through Gen12 (Tiger Lake / Alder Lake / Iris Xe) Command Reference and Video Engine specification.

### 2.2 Device & Generation Identification
Intel GPUs are identified via PCI Vendor ID `0x8086`. Media capabilities are strictly partitioned by architectural generation:

| Generation | Representative Platform | PCI Device IDs | Media Engine Architecture | Codec Capabilities |
|---|---|---|---|---|
| **Gen7 / Gen7.5** | Haswell (H81 Target) | `0x0402`, `0x0412`, `0x0416`, `0x041E` | MFX (Multi-Format Codec Engine) | H.264 (AVC) Baseline/Main/High up to 4K; MPEG-2; VC-1 |
| **Gen8** | Broadwell | `0x1602`, `0x1616`, `0x1626` | MFX + early VP8 | H.264 4K; VP8; Partial HEVC 8-bit hybrid |
| **Gen9** | Skylake, Kaby Lake, Coffee Lake | `0x1912`, `0x5912`, `0x3E92`, `0x9BC5` | MFX + HCP (HEVC/VP9 Decoder) | H.264 4K; HEVC Main/Main10 (4K@60); VP9 Profile 0/2 (4K); VP8 |
| **Gen11** | Ice Lake | `0x8A52`, `0x8A51` | Dual VDBox + HCP | H.264, HEVC 10-bit 4:2:2/4:4:4, VP9 10-bit 4:4:4, HDR10 |
| **Gen12 / Xe-LP** | Tiger Lake, Alder Lake, Iris Xe | `0x9A49`, `0x4680`, `0x4690` | Multi-pipe VDBox (VCS0..VCS3) | AV1 8/10-bit up to 8K@60; HEVC 12-bit; VP9 12-bit; H.264 |
| **Xe-HPG / Xe2** | Alchemist (Arc), Battlemage | `0x5690`, `0x56A0`, `0xE202` | Independent Media Engine | AV1 Encode/Decode; 8K60 10-bit; Dual Hardware Encoders |

### 2.3 Hardware Media Engines & VDBox
- **MFX Engine (Multi-Format Codec)**: Found in Gen7–Gen8. Processes H.264, MPEG-2, and VC-1. Uses a hardware entropy decoder (BSD - Bitstream Decoder) and motion compensation unit.
- **HCP Engine (HEVC/VP9)**: Introduced in Gen9. Specialized fixed-function pipeline for CTU (Coding Tree Unit) processing, CABAC entropy parsing, and loop filtering (Deblocking + SAO).
- **VDBOX (Video Decoder Box)**: Starting in Gen8/Gen9, the video decoder is packaged as independent hardware rings (`VCS0`, `VCS1`). High-end dies contain multiple parallel VDBox instances.
- **Microcontrollers (GuC & HuC)**:
  - **GuC (Graphics MicroController)**: Manages low-latency queue scheduling and multi-engine workload dispatch.
  - **HuC (Hardware User Controller)**: An on-die HE-AAC / security microcontroller used in Gen9+ to authenticate and parse bitstream slice headers offloading CPU involvement. In BOS, basic hardware decode can run in direct MMIO/Ring submission mode without mandatory HuC authentication unless content protection is required.

### 2.4 Command Submission Model
Intel media command submission uses GPU command rings and batch buffers:
1. **Ring Buffer**: Memory mapped ring buffer with Head (`0x1C000 + 0x34`) and Tail (`0x1C000 + 0x30`) pointers in MMIO.
2. **Command Primitives**:
   - `MI_BATCH_BUFFER_START (0x31 << 23)`: Dispatches a command buffer mapped in GTT.
   - `MI_FLUSH_DW (0x26 << 23)`: Writes completion timestamps and flushes caches to surface memory.
   - `MI_USER_INTERRUPT (0x02 << 23)`: Fires an MSI/legacy interrupt to signal frame decode completion.
3. **Pipeline Packets**:
   - `MFX_PIPE_MODE_SELECT`: Selects codec standard (AVC, MPEG-2, VC-1).
   - `MFX_SURFACE_STATE`: Defines destination Y and UV pitch, tiling, and base address.
   - `MFX_PIPE_BUF_ADDR_STATE`: Configures decoded picture buffer (DPB) references.
   - `MFX_IND_OBJ_BASE_ADDR_STATE`: Configures bitstream compressed payload base physical address.
   - `MFX_AVC_IMG_STATE`: Configures picture dimensions, macroblock dimensions, and chroma format.
   - `MFX_AVC_SLICE_STATE`: Supplies slice header parameters (NAL type, slice Qp, reference lists).
   - `MFD_AVC_BSD_OBJECT`: Commands the hardware BSD unit to consume $N$ bytes from the bitstream buffer.

### 2.5 Surface Management & Tiling
Intel media pipelines require specific memory layouts:
- **Tiling Formats**:
  - **Linear**: Simple row-major order. Supported for CPU access and legacy display scanout.
  - **Tile-X**: 512x8 byte cacheline tiling. Used for legacy scanout surfaces.
  - **Tile-Y**: 128x32 byte cacheline tiling. **Mandatory** for Gen7–Gen11 VDBox output surfaces for memory bandwidth conservation.
  - **Tile-4**: Introduced in Gen12/Xe-LP for modern memory controllers.
- **Pixel Formats**:
  - **NV12**: 8-bit Y plane followed by an interleaved UV plane at half height. Default for all 8-bit decoders.
  - **P010**: 10-bit Y plane followed by interleaved UV plane (each sample stored in the upper 10 bits of a 16-bit word).

### 2.6 Mapping to BOS Intel Backend
```c
/* kernel/media/bospectra/decoder/video_accel/intel/intel_accel.h */
typedef struct {
    uint16_t device_id;
    uint32_t gen;
    uint64_t mmio_base;
    uint64_t mmio_size;
    uint64_t ggtt_base;
    uint32_t vcs_ring_tail;
    bool     has_hcp;
    bool     has_av1;
} bos_intel_video_ctx_t;

bos_video_status_t bos_intel_video_detect(bos_video_device_info_t* dev_info);
bos_video_status_t bos_intel_video_query_caps(bos_video_caps_t* caps);
bos_video_status_t bos_intel_video_create_decoder(bos_video_decoder_desc_t* desc, void** out_ctx);
bos_video_status_t bos_intel_video_submit_picture(void* ctx, const bos_video_picture_t* pic);
bos_video_status_t bos_intel_video_get_surface(void* ctx, bos_video_surface_t** out_surf);
bos_video_status_t bos_intel_video_sync(void* ctx, uint64_t timeout_us);
bos_video_status_t bos_intel_video_destroy(void* ctx);
```

---

## 3. Forensic Study 2: NVIDIA Hardware Video Architecture

### 3.1 Reference Sources
- **NVIDIA NVDEC Video Decoder API Programming Guide**: Official specification of NVIDIA's fixed-function hardware video decoding pipeline.
- **NVIDIA open-gpu-kernel-modules**: Open-source kernel driver modules (Dual MIT/GPL-2.0, `https://github.com/NVIDIA/open-gpu-kernel-modules`).
- **NVIDIA CUDA Video Codec SDK**: Interface structures (`cuviddec.h`, `nvcuvid.h`).

### 3.2 Stack Decomposition: Kernel vs Firmware vs User API vs Hardware Engine
A critical forensic distinction must be maintained when evaluating NVIDIA:

```
┌────────────────────────────────────────────────────────────────────────┐
│                      APPLICATION / MEDIA PIPELINE                      │
│                  BOSpectra / Media Player Application                  │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│                     USER-SPACE VIDEO API (NVDEC)                       │
│  - Bitstream Parser (cuvidParseVideoData)                              │
│  - Decoder Session Management (cuvidCreateDecoder)                     │
│  - Picture Description Packet Builder (CUVIDPICPARAMS)                 │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │ Command Pushbuffer
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│                      GPU DRIVER (Kernel / Control)                     │
│  - BAR0 MMIO Register Access (0xFD000000 / Physical)                  │
│  - GMMU (GPU Virtual Memory Page Tables & Unified Memory)              │
│  - Channel & Engine Allocation (FIFO Runlists)                         │
│  - GSP (GPU System Processor) Firmware Communication (Turing / Ada)    │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │ DMA Ring Dispatch
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│                     ON-DIE HARDWARE VIDEO ENGINE                       │
│  - NVDEC Fixed-Function Processor (Separate from CUDA Shader Cores)    │
│  - Bitstream Syntax Demux & Entropy Decoding (CABAC/CAVLC)             │
│  - Motion Vector & Spatial Reconstruction                             │
│  - In-Loop Filter (Deblocking / SAO / CDEF)                            │
│  - Direct VRAM DMA write to NV12 / P010 Surface                        │
└────────────────────────────────────────────────────────────────────────┘
```

1. **Kernel Driver Role**: The open kernel modules (`nvidia.ko` / `open-gpu-kernel-modules`) do **not** decode video. They manage physical PCIe BARs, allocate VRAM buffers, program the GMMU, and submit work packets to GPU execution channels.
2. **GSP Firmware Role**: On Turing (RTX 20), Ampere (RTX 30), and Ada Lovelace (RTX 40), GPU control functions (clock gating, engine power, channel sequencing) are governed by on-die RISC-V GSP firmware.
3. **NVDEC Hardware Role**: NVDEC is a dedicated silicon block completely decoupled from CUDA SMs (Streaming Multiprocessors). Playing an 8K video on NVDEC consumes 0% 3D/CUDA shader load.

### 3.3 NVDEC Generation & Codec Support Matrix

| Microarchitecture | Family Examples | Max Streams | Supported Decoders | Max Resolution |
|---|---|---|---|---|
| **Kepler** | GTX 600 / 700 | 1 | MPEG-2, VC-1, H.264 | 4096x2160 (4K) |
| **Maxwell (GM206)** | GTX 960 / 950 | 2 | H.264, HEVC 8-bit/10-bit, VP8 | 4096x2160 (4K) |
| **Pascal** | GTX 10-Series (GP102-GP108) | 2 | H.264, HEVC 10-bit/12-bit, VP8, VP9 Profile 0/2 | 8192x8192 (8K) |
| **Turing** | RTX 20-Series, GTX 16-Series | 2–3 | H.264, HEVC 10/12-bit, VP8, VP9, HEVC 4:4:4 | 8192x8192 (8K) |
| **Ampere** | RTX 30-Series (GA102-GA106) | 3–4 | H.264, HEVC, VP9, **AV1 Main Profile (8K@60)** | 8192x8192 (8K) |
| **Ada Lovelace** | RTX 40-Series (AD102-AD106) | 4–5 | Dual NVDEC: H.264, HEVC, VP9, AV1, 10-bit P010 | 8192x8192 (8K) |

### 3.4 Operational Flow & Picture Description Submissions
To execute hardware decode, NVDEC requires:
1. **Parser Execution**: The host demuxes the elementary stream and extracts sequence headers (SPS/PPS/VPS).
2. **Session Creation**: Allocates decoder surfaces in VRAM based on bit depth, chroma format, and DPB size.
3. **Picture Parameter Assembly**:
   - `CUVIDPICPARAMS`: Frame dimensions, picture structure (progressive vs interlaced), current surface index, reference surface indices (`ref_pic_flag`).
   - Codec-specific structures (`CUVIDH264PICPARAMS`, `CUVIDHEVCPICPARAMS`, `CUVIDAV1PICPARAMS`).
   - Bitstream slice data pointer and byte length.
4. **Channel Dispatch**: The packet is enqueued to the NVDEC FIFO channel.
5. **Surface Mapping & Synchronization**:
   - Query completion fence via GPU semaphore.
   - Map decoded NV12/P010 surface base address in VRAM.

### 3.5 Mapping to BOS NVIDIA Backend
```c
/* kernel/media/bospectra/decoder/video_accel/nvidia/nvidia_accel.h */
typedef struct {
    uint16_t device_id;
    uint32_t architecture;       /* PASCAL, TURING, AMPERE, ADA */
    uint64_t bar0_mmio_phys;
    uint64_t bar1_vram_phys;
    void*    mmio_virt;
    uint32_t nvdec_channel_id;
    uint32_t dpb_surface_count;
    bool     supports_av1;
} bos_nvidia_video_ctx_t;

bos_video_status_t bos_nvidia_video_detect(bos_video_device_info_t* dev_info);
bos_video_status_t bos_nvidia_video_query_caps(bos_video_caps_t* caps);
bos_video_status_t bos_nvidia_video_create_decoder(bos_video_decoder_desc_t* desc, void** out_ctx);
bos_video_status_t bos_nvidia_video_submit_picture(void* ctx, const bos_video_picture_t* pic);
bos_video_status_t bos_nvidia_video_get_surface(void* ctx, bos_video_surface_t** out_surf);
bos_video_status_t bos_nvidia_video_sync(void* ctx, uint64_t timeout_us);
bos_video_status_t bos_nvidia_video_destroy(void* ctx);
```

---

## 4. Forensic Study 3: AMD Hardware Video Architecture

### 4.1 Reference Sources
- **AMD rocDecode**: Modern C++ hardware video decoding API for AMD GPUs (MIT License, `https://github.com/ROCm/rocDecode`).
- **AMD VCN (Video Core Next) Specification**: Open documentation in Mesa / AMDGPU driver.
- **AMDGPU Linux Driver**: Kernel ring buffer management and packet formatting.

### 4.2 AMD Video Core Next (VCN) Evolution
AMD unified video decoding and encoding under the **VCN** hardware engine, replacing the legacy UVD (Unified Video Decoder) and VCE (Video Coding Engine):

| VCN Version | Architecture Generation | Typical Products | Codec Hardware Capabilities |
|---|---|---|---|
| **VCN 1.0** | Raven Ridge (GCN 5.0) | Ryzen 2000 APUs | MPEG-2, H.264 (4K), HEVC 8/10-bit (4K), VP9 8/10-bit (4K) |
| **VCN 2.0** | Navi 1x (RDNA 1.0) | RX 5500, 5600, 5700 | H.264 4K, HEVC 4K@60, VP9 4K@60 |
| **VCN 2.2** | Renoir / Cezanne | Ryzen 4000 / 5000 APUs | Enhanced HEVC / VP9 decode power efficiency |
| **VCN 3.0** | Navi 2x (RDNA 2.0) | RX 6000 Series | H.264 4K, HEVC 8K, VP9 8K, **AV1 8K@60 8b/10b** |
| **VCN 4.0** | Navi 3x (RDNA 3.0) | RX 7000 Series | Dual VCN Engines, AV1 8K@120, Advanced B-frame support |

### 4.3 Command Submission Model: Indirect Buffers (IB)
1. **Ring Buffer Allocation**: AMDGPU reserves dedicated ring space for `VCN_DEC_0` (and `VCN_DEC_1` on multi-engine dies).
2. **Indirect Buffers (IB)**: Workloads are packaged into Indirect Buffers in system or VRAM memory:
   - **Context Session Packet**: Establishes stream handle and codec mode (`VCN_CODEC_H264`, `VCN_CODEC_HEVC`, `VCN_CODEC_AV1`).
   - **Bitstream Target Packet**: Specifies physical base address and byte size of compressed elementary slices.
   - **DPB Reference Picture List**: Hardware surface buffer IDs for reconstructed and forward/backward reference frames.
   - **Output Surface Target**: Physical VRAM base address for uncompressed NV12 or P010 destination buffer.
3. **Doorbell / WPTR Dispatch**: The host writes the new packet offset to the VCN Write Pointer register or PCIe Doorbell page, triggering immediate execution.
4. **Fence Event**: The hardware writes an incremental 32-bit sequence number to a designated host-visible memory location and triggers an interrupt upon completion.

### 4.4 Mapping to BOS AMD Backend
```c
/* kernel/media/bospectra/decoder/video_accel/amd/amd_accel.h */
typedef struct {
    uint16_t device_id;
    uint32_t vcn_version;        /* VCN_1_0, VCN_2_0, VCN_3_0, VCN_4_0 */
    uint64_t mmio_phys;
    uint64_t vram_phys;
    void*    mmio_virt;
    uint32_t ring_wptr;
    uint32_t stream_handle;
    bool     supports_av1;
} bos_amd_video_ctx_t;

bos_video_status_t bos_amd_video_detect(bos_video_device_info_t* dev_info);
bos_video_status_t bos_amd_video_query_caps(bos_video_caps_t* caps);
bos_video_status_t bos_amd_video_create_decoder(bos_video_decoder_desc_t* desc, void** out_ctx);
bos_video_status_t bos_amd_video_submit_picture(void* ctx, const bos_video_picture_t* pic);
bos_video_status_t bos_amd_video_get_surface(void* ctx, bos_video_surface_t** out_surf);
bos_video_status_t bos_amd_video_sync(void* ctx, uint64_t timeout_us);
bos_video_status_t bos_amd_video_destroy(void* ctx);
```

---

## 5. Forensic Study 4: Common GPU Driver Architecture & BOS Mapping

### 5.1 DRM / GEM / PRIME / DMA-BUF Analysis
Real-world operating systems manage GPU memory and shared hardware pipelines through well-defined abstractions. Studying Linux DRM/GEM as an engineering model reveals the key concepts:

| DRM / GEM Concept | Engineering Purpose | Common Pitfall to Avoid in BOS | BOS-Native Equivalent |
|---|---|---|---|
| **Buffer Object (BO)** | Unit of memory allocated in GPU VRAM or pinned RAM accessible by hardware DMA engines | Do NOT port the complex Linux GEM object refcount graph | `bos_gpu_bo_t`: Simple memory descriptor tracking physical address, size, and domain |
| **Memory Domain** | Specifies whether buffer resides in CPU RAM, GTT aperture, or VRAM | Forgetting cache coherence between CPU writes and GPU reads | `bos_gpu_mem_domain_t` (`DOMAIN_SYSTEM`, `DOMAIN_GTT`, `DOMAIN_VRAM`) |
| **Synchronization Fence** | Monotonic sequence number or completion event signaling when GPU is finished | Synchronously busy-waiting on the CPU without timeout | `bos_gpu_fence_t`: Monotonic sequence counter with timeout checking |
| **dma-buf / PRIME** | Passing a GPU surface from a decoder to a display engine without copying | Creating circular driver dependencies | `bos_gpu_surface_t`: Universal surface descriptor with shared handle |
| **Command Ring / IB** | Circular queue in memory where CPU enqueues instructions for GPU execution | Overflowing ring without tracking hardware Read Pointer (RPTR) | `bos_gpu_ring_t`: Tracked ring with head, tail, and wrap-around protection |

---

## 6. Native BOS Video Acceleration HAL Design

### 6.1 Subsystem Architecture & Plug-In Layer
The new HAL design resides in `kernel/media/bospectra/decoder/video_accel/`:

```
kernel/media/bospectra/decoder/video_accel/
├── include/
│   ├── video_accel.h            # Vendor-neutral public HAL interface
│   ├── video_accel_caps.h       # Detailed hardware capability bitmasks & structures
│   ├── video_accel_surface.h    # GPU & CPU video surface representations
│   └── video_accel_backend.h    # Backend driver registration & ops table
├── common/
│   ├── video_accel_core.c       # Subsystem init, device registry & backend router
│   ├── video_accel_policy.c     # Stream-to-hardware matching & fallback policy
│   └── video_accel_surface.c    # Universal surface lifecycle & format management
├── intel/
│   ├── intel_accel.c            # Intel MFX / VDBox hardware backend
│   └── intel_accel.h            # Intel-specific register definitions
├── nvidia/
│   ├── nvidia_accel.c           # NVIDIA NVDEC hardware backend
│   └── nvidia_accel.h           # NVIDIA-specific pushbuffer & channel structures
├── amd/
│   ├── amd_accel.c              # AMD VCN hardware backend
│   └── amd_accel.h              # AMD-specific VCN IB ring packet structures
└── software/
    ├── software_accel.c         # High-speed multi-threaded CPU fallback backend
    └── software_accel.h         # Connects directly to C99 h264/hevc/vp8/vp9 decoders
```

### 6.2 HAL Function Table Interface
Every vendor backend implements the standardized `bos_video_backend_ops_t` table:

```c
typedef struct bos_video_backend_ops {
    const char* (*get_name)(void);
    bos_video_status_t (*probe)(bos_video_device_t* dev);
    bos_video_status_t (*get_caps)(bos_video_device_t* dev, bos_video_caps_t* out_caps);
    bos_video_status_t (*create_decoder)(bos_video_device_t* dev, const bos_video_config_t* config, void** out_decoder_ctx);
    bos_video_status_t (*submit_packet)(void* decoder_ctx, const bos_video_packet_t* packet);
    bos_video_status_t (*get_frame)(void* decoder_ctx, bos_video_surface_t** out_surface);
    bos_video_status_t (*sync)(void* decoder_ctx, uint64_t timeout_us);
    bos_video_status_t (*flush)(void* decoder_ctx);
    bos_video_status_t (*seek)(void* decoder_ctx, uint64_t seek_pts_us);
    bos_video_status_t (*destroy_decoder)(void* decoder_ctx);
} bos_video_backend_ops_t;
```

---

## 7. Device Capability Model

Every detected video acceleration device reports an exhaustive, non-fabricated capability profile:

```c
typedef struct {
    /* Device Identification */
    uint16_t vendor_id;
    uint16_t device_id;
    char     device_name[64];
    char     architecture_name[32];
    uint32_t hardware_generation;
    uint32_t video_engine_count;

    /* Codec Support Bitmask */
    uint32_t supported_codecs;        /* H264, HEVC, VP8, VP9, AV1, MPEG2 */
    
    /* Granular Codec Profiles */
    struct {
        uint32_t max_width;
        uint32_t max_height;
        uint32_t max_level;
        uint32_t supported_profiles;  /* Baseline, Main, High, Main10, etc. */
        uint8_t  min_bit_depth;       /* 8 */
        uint8_t  max_bit_depth;       /* 10, 12 */
        uint32_t chroma_formats;      /* 4:2:0, 4:2:2, 4:4:4 */
    } codec_limits[BOS_CODEC_MAX];

    /* Surface & Memory Capabilities */
    uint32_t supported_surface_formats; /* NV12, P010, YUV420P, ARGB32 */
    bool     supports_direct_vram_scanout;
    bool     supports_zero_copy_presentation;
    
    /* Engine Operation */
    bool     supports_asynchronous_fences;
    bool     supports_slice_multithreading;
    bool     supports_postprocessing_scaling;
} bos_video_caps_t;
```

---

## 8. Hardware vs Software Decoder Selection Policy

The HAL enforces a deterministic 7-step stream negotiation algorithm before allocating decoder resources:

```
Stream Detected (Codec, Profile, Level, Dimensions, BitDepth)
                        │
                        ▼
           [Step 1] Query Primary GPU Device
                        │
                        ▼
    [Step 2] Is Hardware Backend Available for Vendor?
              ├── NO  ──────────────────────────────────────────┐
              │                                                 │
              ▼ YES                                             │
    [Step 3] Is Codec Supported in Hardware?                   │
              ├── NO  ──────────────────────────────────────────┤
              │                                                 │
              ▼ YES                                             │
    [Step 4] Is Profile Supported in Hardware?                  │
              ├── NO  ──────────────────────────────────────────┤
              │                                                 │
              ▼ YES                                             │
    [Step 5] Is Stream Resolution within Hardware Limits?      │
              ├── NO  ──────────────────────────────────────────┤
              │                                                 │
              ▼ YES                                             │
    [Step 6] Can Hardware Surfaces be Allocated?                │
              ├── NO  ──────────────────────────────────────────┤
              │                                                 │
              ▼ YES                                             ▼
┌───────────────────────────────────────────────┐ ┌───────────────────────────┐
│     ALLOCATE HARDWARE DECODER SESSION         │ │ LOG FALLBACK CAUSE TO KLOG│
│ - Vendor Backend (Intel / NVIDIA / AMD)       │ │ Allocate BOS C99 Software │
│ - Hardware Video Surfaces (NV12 / P010)       │ │ Decoder (H264 / HEVC /    │
│ - Asynchronous Fence Tracking                 │ │ VP8 / VP9)                │
└───────────────────────────────────────────────┘ └───────────────────────────┘
```

### Forensic Fallback Reason Codes
When fallback occurs, the kernel prints an explicit forensic error:
- `[VIDEO_ACCEL_FALLBACK] Reason: NO_GPU_FOUND`
- `[VIDEO_ACCEL_FALLBACK] Reason: VENDOR_BACKEND_UNAVAILABLE (Vendor: 0x10DE)`
- `[VIDEO_ACCEL_FALLBACK] Reason: CODEC_UNSUPPORTED_IN_HW (Codec: AV1, Gen: Haswell Gen7)`
- `[VIDEO_ACCEL_FALLBACK] Reason: PROFILE_UNSUPPORTED (Codec: H264, Profile: High10, HW Max: High 8-bit)`
- `[VIDEO_ACCEL_FALLBACK] Reason: RESOLUTION_EXCEEDS_HW_LIMIT (Stream: 7680x4320, HW Limit: 4096x2160)`
- `[VIDEO_ACCEL_FALLBACK] Reason: VRAM_SURFACE_EXHAUSTION (Requested: 64MB, Available: 12MB)`

---

## 9. GPU Surface Architecture & Zero-Copy Path

### 9.1 Surface Descriptor
```c
typedef enum {
    BOS_SURF_DOMAIN_CPU_RAM = 0,    /* Traditional host heap memory */
    BOS_SURF_DOMAIN_GTT_MAPPED,     /* Pinned system memory accessible by GPU DMA */
    BOS_SURF_DOMAIN_VRAM_LOCAL      /* High-bandwidth on-card VRAM */
} bos_surf_domain_t;

typedef struct bos_video_surface {
    uint32_t          surface_id;
    uint32_t          width;
    uint32_t          height;
    bos_surf_format_t format;       /* NV12, P010, YUV420P */
    bos_surf_domain_t domain;
    
    /* Plane Metrics */
    uint32_t          pitches[3];
    uint32_t          offsets[3];
    uint64_t          phys_addresses[3];
    void*             virt_pointers[3];
    
    /* Hardware Synchronization */
    uint64_t          fence_sequence;
    bool              is_locked_by_hardware;
    uint32_t          reference_count;
} bos_video_surface_t;
```

### 9.2 Zero-Copy Presentation Pipeline
- **Ideal Zero-Copy Path**:
  $$\text{Bitstream} \xrightarrow{\text{DMA}} \text{HW Decoder} \xrightarrow{\text{DMA}} \text{VRAM Surface (NV12)} \xrightarrow{\text{GPU Scanout / Blit}} \text{BOSurface Framebuffer}$$
  *Copy Count*: **0 CPU Copies**.
- **Low-Copy Linear Fallback Path**:
  Where zero-copy scanout is not yet wired to the display controller, the hardware decoder writes to pinned host memory (GTT/coherent DMA), and the software compositor reads directly from the memory aperture.
  *Copy Count*: **1 CPU Read/Blit**.
- **Software Path**:
  Software decoder reconstructs macroblocks into host heap YUV420P buffer, followed by software BT.601/BT.709 color conversion into ARGB32 window canvas.
  *Copy Count*: **2 CPU Operations (Decode + Convert)**.

---

## 10. Asynchronous Synchronization Model

Hardware video decoding is asynchronous. The host CPU must never assume a frame is ready immediately upon submitting bitstream slice packets:

1. **Submission Phase**: `bos_video_accel_submit_packet()` writes the bitstream pointer and commands to the hardware channel and records an incremental `fence_sequence_id`.
2. **Execution Phase**: The hardware decoder executes in parallel with OS scheduling.
3. **Synchronization Check (`bos_video_accel_sync`)**:
   - Checks the hardware sequence register or status page (HWS).
   - If the sequence number is $\ge \text{target\_id}$, the surface state transitions to `SURF_STATE_READY`.
   - If the timeout expires without completion, triggers a hardware watchdog alert and marks the frame as dropped.
4. **Flush & Seek Handling**:
   - On seek or file close, the HAL invokes `bos_video_accel_flush()`.
   - Halts pending ring execution, invalidates unpresented surface locks, resets DPB reference tables, and zeroes the sync fences.

---

## 11. Existing BOS Infrastructure Reuse Audit

Investigation of `kernel/graphics/gpu/` and `kernel/core/pci/` confirms that ATOMS OS already possesses robust low-level foundations that must be reused without duplication:

1. **PCI Discovery**: `pci_get_device_count()` and `pci_get_device()` in `kernel/core/pci/` already discover all Class 0x03 graphics adapters.
2. **GPU Device Tracking**: `gpu_manager_register_pci_device()` in `kernel/graphics/gpu/core/gpu_manager.c` already initializes `bos_gpu_device_t` with BAR0/BAR1 base addresses and IRQ lines.
3. **BAR MMIO Mapping**: `gpu_mem_map_bar()` in `kernel/graphics/gpu/memory/gpu_memory.c` provides identity mapping for physical BAR memory.
4. **Surface Allocations**: `gpu_surface_create()` in `kernel/graphics/gpu/surface/gpu_surface.c` provides the core memory allocation model.

**Reuse Directive**: The Video Acceleration HAL will bind directly into the active `bos_gpu_device_t` instances registered by `gpu_manager.c`, preventing duplicate PCIe bus scans and conflicting MMIO mappings.

---

## 12. License Forensics Audit

A dedicated license analysis was conducted to protect ATOMS OS intellectual property and prevent GPL contagion:

| Subsystem / Reference | Originating Project | Stated License | ATOMS OS Usage Policy | Contagion Risk |
|---|---|---|---|---|
| **Intel Media Driver** | Intel Corporation | MIT License | Permissible clean reference; extraction of register structures & command sequences | **NONE** (Permissive) |
| **NVIDIA open-gpu-kernel**| NVIDIA Corporation | Dual MIT / GPL-2.0 | Reference for BAR0/BAR1 and GSP firmware control protocol; use MIT path only | **NONE** (Under MIT clause) |
| **NVIDIA Video Codec SDK** | NVIDIA Corporation | NVIDIA SDK License (Headers permissive) | Clean-room implementation of parameter structures (`PICPARAMS`) without binary DLL dependencies | **NONE** (Clean-room) |
| **AMD rocDecode** | Advanced Micro Devices | MIT License | Permissible clean reference for VCN sequence parsing | **NONE** (Permissive) |
| **Linux DRM / GEM / i915**| Linux Kernel | GPL-2.0 Only | **STRICTLY PROHIBITED FROM IMPORT**. Used only as high-level conceptual reference | **CRITICAL IF IMPORTED** (Do Not Import) |

*The complete formal license review is maintained in `docs/media/THIRD_PARTY_GPU_LICENSE_AUDIT.md`.*

---

## 13. Telemetry Specification

Every active media session will emit a structured telemetry block to the serial diagnostic log:

```text
======================= BOSPECTRA VIDEO ACCELERATION TELEMETRY =======================
  GPU VENDOR           : Intel Corporation (0x8086)
  GPU DEVICE           : Intel HD Graphics 4400/4600 (Haswell Gen7.5, DevID: 0x041E)
  VIDEO ACCEL BACKEND  : Intel VDBox MFX Backend (Hardware Direct)
  DECODER SELECTED     : H.264 (AVC) Baseline/Main Profile
  DECODE MODE          : HARDWARE ACCELERATED
  INPUT STREAM         : 1920x1080 @ 29.97 FPS, 8-bit YUV 4:2:0, Progressive
  OUTPUT SURFACE       : GPU Memory (Tile-Y NV12, Handle: 0x000040A0)
  MEMORY DOMAIN        : GTT Mapped Host Coherent DMA
  ZERO-COPY SCANOUT    : ACTIVE (Direct Window Surface Handoff)
  FRAMES DECODED       : 1,842
  FRAMES PRESENTED     : 1,842
  FRAMES DROPPED       : 0
  AVERAGE DECODE TIME  : 2.14 ms / frame
  SYNC LATENCY (DRIFT) : +1.2 ms (Within broadcast tolerance -40ms..+15ms)
  FALLBACK COUNT       : 0 (Zero CPU fallbacks)
======================================================================================
```

---

## 14. Hardware Activity Proof Protocol

To prevent manufactured results or false claims of acceleration, the following **8-Point Proof Protocol** is mandated:
1. **Device Identification**: Verified match between PCI vendor/device ID and hardware database.
2. **Backend Selection**: Chosen backend matches physical hardware vendor.
3. **Decoder Session Instantiation**: Hardware-specific channel/context successfully allocated.
4. **Real Bitstream Consumption**: Bitstream bytes from media file container are submitted into hardware buffers.
5. **Hardware Engine Execution**: Non-zero hardware completion counter / fence sequence increment.
6. **Surface Verification**: Real non-zero Y and UV plane pixel data in hardware surface format (NV12/P010).
7. **Presentation Confirmation**: Surface scanout address blitted to display buffer.
8. **Telemetry Audit**: Output of frames decoded, decode duration, and zero synthetic fallback flags.

---

## 15. Required Vendor / Codec Test Matrix

This matrix establishes the universal testing baseline. Every cell must strictly contain one of: `HW-PASS`, `SW-FALLBACK-PASS`, `UNSUPPORTED`, or `NOT-TESTED`.

| Codec Standard | NVIDIA Backend (NVDEC) | Intel Backend (VDBox) | AMD Backend (VCN) | Software Fallback |
|---|---|---|---|---|
| **H.264 (AVC)** | NOT-TESTED | NOT-TESTED | NOT-TESTED | **SW-FALLBACK-PASS** |
| **HEVC / H.265** | NOT-TESTED | NOT-TESTED | NOT-TESTED | **SW-FALLBACK-PASS** |
| **VP8** | NOT-TESTED | NOT-TESTED | NOT-TESTED | **SW-FALLBACK-PASS** |
| **VP9** | NOT-TESTED | NOT-TESTED | NOT-TESTED | **SW-FALLBACK-PASS** |
| **AV1** | NOT-TESTED | NOT-TESTED | NOT-TESTED | NOT-TESTED |
| **MPEG-2** | NOT-TESTED | NOT-TESTED | NOT-TESTED | **SW-FALLBACK-PASS** |

*Note*: Cells currently marked `NOT-TESTED` will transition to `HW-PASS` only when the specific hardware backend has been physically tested on bare metal and verified with the 8-Point Proof Protocol.

---

## 16. M4 Certification Taxonomy

To prevent premature claims of completion, all subsystems must report one of four certified states:

1. **`IMPLEMENTED`**: Code is written, structurally complete, and compiles without warnings or errors.
2. **`HOST-VALIDATED`**: Architecture logic, parsing math, capability negotiation, and fallback policies pass unit tests on the host development toolchain.
3. **`RUNTIME-VALIDATED`**: System boots in UEFI mode under QEMU; hardware probing, software fallback, and telemetry logging execute without crashing.
4. **`PHYSICALLY-VALIDATED`**: Code runs on bare-metal physical hardware with genuine hardware engine execution verified by the 8-Point Proof Protocol.

---

## 17. Physical Hardware Target Profiles

1. **Target 1: H81 Physical Testbed (Haswell)**:
   - **CPU**: Intel Core i3 4th Gen (Haswell x86_64).
   - **Integrated GPU**: Intel HD Graphics 4400 / 4600 (Gen7.5 MFX Engine).
   - **Discrete GPU**: NVIDIA GeForce RTX 4060 (Ada Lovelace Dual NVDEC).
   - **RAM**: 8 GB DDR3.
   - **Firmware**: 2022 UEFI Firmware.
2. **Target 2: QEMU Emulated Reference**:
   - Pure UEFI OVMF with standard VGA / QEMU xHCI / Intel HDA.
   - Validates software fallback, container demuxing, and HAL architecture stability.

---

## 18. Known Limitations & Forensic Risk Analysis

1. **Firmware Blobs**:
   - Modern GPUs (Intel Gen9+ HuC/GuC, NVIDIA Turing+ GSP, AMD VCN) require signed proprietary firmware microcode loaded into GPU RAM before execution channels activate.
   - *Mitigation*: Prioritize direct MMIO ring submission without firmware authentication where supported (e.g. Intel Gen7/Haswell MFX), or integrate permissive firmware loaders for modern discrete GPUs.
2. **Memory Domain Translation**:
   - Mapping physical VRAM directly to userspace requires PML4/PDPT page table entries with Write-Combining (`PAT`) attributes to prevent tearing.
   - *Mitigation*: Utilize the existing kernel `vmm_map_page()` infrastructure with `PAGE_WRITABLE` and cache-disable flags.
3. **Display Scanout Integration**:
   - Direct zero-copy display requires the video acceleration surface address to be accepted directly by the display engine plane controller without CPU re-blit.
   - *Mitigation*: Start with low-copy GTT host-coherent read path, transitioning to direct display plane flip once the display engine modesetting interface is unified.

---

## 19. Next Steps in Phase Isolation

Per ATOMS OS Engineering Protocol Rule 0:
1. **Task 1 (This Audit)**: Submitted for architectural review and sign-off.
2. **Task 2 (Architecture & Patch Plan)**: Upon approval, formulate `docs/media/GPU_VIDEO_ACCELERATION_PATCH_PLAN.md`.
3. **Task 3 (Patch Execution)**: Implement the modular `video_accel/` directory layout and vendor backend hooks.
4. **Task 4 (Certification)**: Execute host validation and physical bare-metal hardware testing.
